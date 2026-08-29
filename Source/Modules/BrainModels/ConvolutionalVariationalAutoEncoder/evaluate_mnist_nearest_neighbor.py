#!/usr/bin/env python3

"""Evaluate MNIST labels from CVAE top codes."""

from __future__ import annotations

import argparse
import csv
import math
from collections import Counter
from pathlib import Path

import numpy as np


EPSILON = 1.0e-12


def repository_root() -> Path:
    return Path(__file__).resolve().parents[4]


def read_codes(path: Path) -> tuple[list[int], list[list[float]]]:
    labels: list[int] = []
    codes: list[list[float]] = []
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            raise RuntimeError(f"{path} has no header")
        code_columns = [
            name for name in reader.fieldnames
            if name.startswith("top_code:")
        ]
        if not code_columns:
            raise RuntimeError(f"{path} has no top_code columns")
        code_columns.sort(key=lambda name: int(name.rsplit(":", 1)[1]))

        for row in reader:
            label = int(round(float(row["label"])))
            code = [float(row[column]) for column in code_columns]
            if not all(math.isfinite(value) for value in code):
                continue
            if all(value == 0 for value in code):
                continue
            labels.append(label)
            codes.append(code)

    if not labels:
        raise RuntimeError(f"{path} did not contain any usable code rows")
    return labels, codes


def nearest_label(reference_labels: list[int],
                  reference_codes: list[list[float]],
                  query_code: list[float]) -> int:
    best_label = reference_labels[0]
    best_distance = float("inf")
    for label, code in zip(reference_labels, reference_codes):
        distance = 0.0
        for query_value, reference_value in zip(query_code, code):
            difference = query_value - reference_value
            distance += difference * difference
            if distance >= best_distance:
                break
        if distance < best_distance:
            best_distance = distance
            best_label = label
    return best_label


def zscore_codes(train_codes: list[list[float]],
                 test_codes: list[list[float]]) -> tuple[list[list[float]], list[list[float]]]:
    dimensions = len(train_codes[0])
    means = [0.0 for _ in range(dimensions)]
    for code in train_codes:
        for index, value in enumerate(code):
            means[index] += value
    means = [value / len(train_codes) for value in means]

    variances = [0.0 for _ in range(dimensions)]
    for code in train_codes:
        for index, value in enumerate(code):
            centered = value - means[index]
            variances[index] += centered * centered
    scales = [
        math.sqrt(value / len(train_codes)) if value > EPSILON else 1.0
        for value in variances
    ]

    def normalize(codes: list[list[float]]) -> list[list[float]]:
        return [
            [
                (value - means[index]) / scales[index]
                for index, value in enumerate(code)
            ]
            for code in codes
        ]

    return normalize(train_codes), normalize(test_codes)


def linear_ridge_predictions(train_labels: list[int],
                             train_codes: list[list[float]],
                             test_codes: list[list[float]],
                             regularization: float) -> list[int]:
    train_matrix = np.asarray(train_codes, dtype=np.float64)
    test_matrix = np.asarray(test_codes, dtype=np.float64)
    train_design = np.hstack(
        [train_matrix, np.ones((train_matrix.shape[0], 1), dtype=np.float64)]
    )
    test_design = np.hstack(
        [test_matrix, np.ones((test_matrix.shape[0], 1), dtype=np.float64)]
    )
    targets = np.zeros((len(train_labels), 10), dtype=np.float64)
    for row, label in enumerate(train_labels):
        if 0 <= label < 10:
            targets[row, label] = 1.0

    penalty = np.eye(train_design.shape[1], dtype=np.float64) * regularization
    penalty[-1, -1] = 0.0
    weights = np.linalg.solve(
        train_design.T @ train_design + penalty,
        train_design.T @ targets,
    )
    scores = test_design @ weights
    return [int(label) for label in np.argmax(scores, axis=1)]


def write_confusion(path: Path, confusion: list[list[int]]) -> None:
    with path.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["actual\\predicted", *range(10)])
        for label, row in enumerate(confusion):
            writer.writerow([label, *row])


def parse_args() -> argparse.Namespace:
    root = repository_root()
    default_dir = root / "UserData" / "cvae_mnist_evaluation"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--train-codes",
        type=Path,
        default=default_dir / "mnist_train_top_codes.csv",
    )
    parser.add_argument(
        "--test-codes",
        type=Path,
        default=default_dir / "mnist_test_top_codes.csv",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=root / "UserData" / "output" / "cvae_mnist_nearest_neighbor",
    )
    parser.add_argument(
        "--normalize",
        choices=["none", "zscore"],
        default="none",
        help="Normalize code dimensions before nearest-neighbour search.",
    )
    parser.add_argument(
        "--classifier",
        choices=["nearest", "ridge"],
        default="nearest",
        help="Classifier probe to use on the frozen codes.",
    )
    parser.add_argument(
        "--ridge-regularization",
        type=float,
        default=1.0e-3,
        help="L2 regularization used by the ridge linear classifier.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    train_labels, train_codes = read_codes(args.train_codes)
    test_labels, test_codes = read_codes(args.test_codes)
    if args.normalize == "zscore":
        train_codes, test_codes = zscore_codes(train_codes, test_codes)

    if args.classifier == "nearest":
        predictions = [
            nearest_label(train_labels, train_codes, code)
            for code in test_codes
        ]
    else:
        predictions = linear_ridge_predictions(
            train_labels,
            train_codes,
            test_codes,
            args.ridge_regularization,
        )

    confusion = [[0 for _ in range(10)] for _ in range(10)]
    correct = 0
    for actual, predicted in zip(test_labels, predictions):
        if predicted == actual:
            correct += 1
        if 0 <= actual < 10 and 0 <= predicted < 10:
            confusion[actual][predicted] += 1

    accuracy = correct / len(test_labels)
    train_distribution = Counter(train_labels)
    test_distribution = Counter(test_labels)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    summary_path = args.output_dir / "summary.csv"
    with summary_path.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["metric", "value"])
        writer.writerow(["train_samples", len(train_labels)])
        writer.writerow(["test_samples", len(test_labels)])
        writer.writerow(["correct", correct])
        writer.writerow(["accuracy", f"{accuracy:.6f}"])
        writer.writerow(["normalization", args.normalize])
        writer.writerow(["classifier", args.classifier])
        if args.classifier == "ridge":
            writer.writerow(["ridge_regularization", args.ridge_regularization])

    write_confusion(args.output_dir / "confusion.csv", confusion)

    print(f"Train samples: {len(train_labels)}")
    print(f"Test samples: {len(test_labels)}")
    print(f"Correct: {correct}")
    print(f"Accuracy: {accuracy:.3%}")
    print(f"Normalization: {args.normalize}")
    print(f"Classifier: {args.classifier}")
    print("Train label distribution:", dict(sorted(train_distribution.items())))
    print("Test label distribution:", dict(sorted(test_distribution.items())))
    print(f"Wrote {summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
