#!/usr/bin/env python3

"""Evaluate MNIST labels from CVAE top-code nearest neighbours."""

from __future__ import annotations

import argparse
import csv
import math
from collections import Counter
from pathlib import Path


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
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    train_labels, train_codes = read_codes(args.train_codes)
    test_labels, test_codes = read_codes(args.test_codes)

    confusion = [[0 for _ in range(10)] for _ in range(10)]
    correct = 0
    predictions: list[int] = []
    for actual, code in zip(test_labels, test_codes):
        predicted = nearest_label(train_labels, train_codes, code)
        predictions.append(predicted)
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

    write_confusion(args.output_dir / "confusion.csv", confusion)

    print(f"Train samples: {len(train_labels)}")
    print(f"Test samples: {len(test_labels)}")
    print(f"Correct: {correct}")
    print(f"Accuracy: {accuracy:.3%}")
    print("Train label distribution:", dict(sorted(train_distribution.items())))
    print("Test label distribution:", dict(sorted(test_distribution.items())))
    print(f"Wrote {summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
