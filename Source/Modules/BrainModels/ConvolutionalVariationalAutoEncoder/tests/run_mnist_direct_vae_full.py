#!/usr/bin/env python3

"""Evaluate constant latent gating on the full centered MNIST dataset."""

from __future__ import annotations

import argparse
import csv
import gzip
import json
import struct
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import run_mnist_direct_vae_sweep as sweep


FULL_DATA_ROOT = sweep.USER_DATA / "cvae_mnist_centered_32_full"
FULL_OUTPUT_ROOT = sweep.USER_DATA / "output" / "cvae_mnist_direct_vae_full"
RAW_ROOT = sweep.USER_DATA / "cvae_mnist" / "raw"
TRAIN_COUNT = 60_000
VALIDATION_COUNT = 10_000


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=600_000)
    parser.add_argument("--replicates", type=int, default=3)
    parser.add_argument("--seed-base", type=int, default=71_000)
    parser.add_argument("--agent", required=True)
    parser.add_argument("--print-tick-interval", type=int, default=60_000)
    parser.add_argument("--resume", action="store_true")
    return parser.parse_args()


def read_idx_images(path: Path, count: int) -> np.ndarray:
    with gzip.open(path, "rb") as handle:
        magic, available, rows, columns = struct.unpack(">IIII", handle.read(16))
        if magic != 2051 or count > available:
            raise RuntimeError(f"Invalid or undersized MNIST image file: {path}")
        data = handle.read(count * rows * columns)
    return np.frombuffer(data, dtype=np.uint8).reshape(count, rows, columns)


def read_idx_labels(path: Path, count: int) -> np.ndarray:
    with gzip.open(path, "rb") as handle:
        magic, available = struct.unpack(">II", handle.read(8))
        if magic != 2049 or count > available:
            raise RuntimeError(f"Invalid or undersized MNIST label file: {path}")
        data = handle.read(count)
    return np.frombuffer(data, dtype=np.uint8)


def center_and_pad(image: np.ndarray) -> np.ndarray:
    weights = image.astype(np.float64)
    total = float(np.sum(weights))
    centered = np.zeros_like(image)
    if total == 0.0:
        centered[:] = image
    else:
        rows, columns = image.shape
        row_coordinate = float(np.sum(weights * np.arange(rows)[:, None]) / total)
        column_coordinate = float(np.sum(weights * np.arange(columns)[None, :]) / total)
        row_shift = int(round((rows - 1) / 2.0 - row_coordinate))
        column_shift = int(round((columns - 1) / 2.0 - column_coordinate))
        source_row_begin = max(0, -row_shift)
        source_row_end = min(rows, rows - row_shift)
        source_column_begin = max(0, -column_shift)
        source_column_end = min(columns, columns - column_shift)
        target_row_begin = source_row_begin + row_shift
        target_row_end = source_row_end + row_shift
        target_column_begin = source_column_begin + column_shift
        target_column_end = source_column_end + column_shift
        centered[target_row_begin:target_row_end, target_column_begin:target_column_end] = image[
            source_row_begin:source_row_end,
            source_column_begin:source_column_end,
        ]
    padded = np.zeros((32, 32), dtype=np.uint8)
    padded[2:30, 2:30] = centered
    return padded


def write_pgm(path: Path, image: np.ndarray) -> None:
    with path.open("wb") as handle:
        handle.write(b"P5\n32 32\n255\n")
        handle.write(image.tobytes())


def prepare_split(
    split: str,
    count: int,
    image_archive: str,
    label_archive: str,
) -> None:
    target = FULL_DATA_ROOT / split
    completion_marker = target / ".complete"
    if completion_marker.exists():
        if completion_marker.read_text().strip() == f"count={count}":
            return

    target.mkdir(parents=True, exist_ok=True)
    images = read_idx_images(RAW_ROOT / image_archive, count)
    labels = read_idx_labels(RAW_ROOT / label_archive, count)
    for index, image in enumerate(images):
        write_pgm(target / f"image_{index:05d}.pgm", center_and_pad(image))
    with (target / "labels.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["label"])
        writer.writerows((int(label),) for label in labels)
    completion_marker.write_text(f"count={count}\n")


def prepare_dataset() -> None:
    prepare_split(
        "train",
        TRAIN_COUNT,
        "train-images-idx3-ubyte.gz",
        "train-labels-idx1-ubyte.gz",
    )
    prepare_split(
        "test",
        VALIDATION_COUNT,
        "t10k-images-idx3-ubyte.gz",
        "t10k-labels-idx1-ubyte.gz",
    )


def configure_sweep() -> None:
    sweep.DATA_ROOT = FULL_DATA_ROOT
    sweep.DATASET_DIRECTORY = FULL_DATA_ROOT.name
    sweep.OUTPUT_ROOT = FULL_OUTPUT_ROOT
    sweep.OUTPUT_DIRECTORY = "output/cvae_mnist_direct_vae_full"
    sweep.TRAIN_COUNT = TRAIN_COUNT
    sweep.VALIDATION_COUNT = VALIDATION_COUNT
    sweep.EXPORT_GATE_VALUES = False
    sweep.EXPORT_CLUSTER_ASSIGNMENT = False


def condition() -> sweep.Condition:
    return sweep.Condition(
        "constant_gate_005",
        "full_dataset",
        "Full centered MNIST with constant gate penalty 0.005",
        {
            "latent_size": "64",
            "beta": "0.03",
            "latent_gating": "yes",
            "latent_gate_penalty": "0.005",
            "latent_gate_warmup_updates": "0",
            "latent_gate_ramp_updates": "0",
            "latent_gate_freeze_update": "0",
        },
    )


def load_normalized_codes(
    run_dir: Path,
) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    train_rows, _ = sweep.evaluation.read_and_align_rows(
        run_dir / "train_codes.csv", FULL_DATA_ROOT / "train" / "labels.csv"
    )
    validation_rows, _ = sweep.evaluation.read_and_align_rows(
        run_dir / "validation_codes.csv", FULL_DATA_ROOT / "test" / "labels.csv"
    )
    train_labels = sweep.evaluation.labels_from_rows(train_rows)
    validation_labels = sweep.evaluation.labels_from_rows(validation_rows)
    train_codes = sweep.evaluation.column_matrix(train_rows, "latent_mean")
    validation_codes = sweep.evaluation.column_matrix(validation_rows, "latent_mean")
    train_codes, validation_codes = sweep.evaluation.zscore(train_codes, validation_codes)
    return train_codes, train_labels, validation_codes, validation_labels


def ten_prototype_accuracy(run_dir: Path) -> float:
    train_codes, train_labels, validation_codes, validation_labels = (
        load_normalized_codes(run_dir)
    )
    prototypes = np.stack(
        [np.mean(train_codes[train_labels == label], axis=0) for label in range(10)]
    )
    distances = (
        np.sum(validation_codes * validation_codes, axis=1, keepdims=True)
        + np.sum(prototypes * prototypes, axis=1)[None, :]
        - 2.0 * validation_codes @ prototypes.T
    )
    predictions = np.argmin(distances, axis=1)
    return sweep.evaluation.accuracy(predictions, validation_labels)


def five_nearest_neighbour_accuracy(run_dir: Path) -> float:
    train_codes, train_labels, validation_codes, validation_labels = (
        load_normalized_codes(run_dir)
    )
    train_squared = np.sum(train_codes * train_codes, axis=1)
    predictions: list[int] = []
    for begin in range(0, len(validation_codes), 64):
        batch = validation_codes[begin:begin + 64]
        distances = (
            np.sum(batch * batch, axis=1, keepdims=True)
            + train_squared[None, :]
            - 2.0 * batch @ train_codes.T
        )
        neighbour_indices = np.argpartition(distances, 4, axis=1)[:, :5]
        neighbour_distances = np.take_along_axis(distances, neighbour_indices, axis=1)
        order = np.argsort(neighbour_distances, axis=1)
        neighbour_indices = np.take_along_axis(neighbour_indices, order, axis=1)
        neighbour_labels = train_labels[neighbour_indices]
        for labels in neighbour_labels:
            counts = np.bincount(labels, minlength=10)
            maximum = int(np.max(counts))
            predictions.append(next(int(label) for label in labels if counts[label] == maximum))
    return sweep.evaluation.accuracy(
        np.asarray(predictions, dtype=np.int64), validation_labels
    )


def squared_distances(points: np.ndarray, centers: np.ndarray) -> np.ndarray:
    return np.maximum(
        np.sum(points * points, axis=1, keepdims=True)
        + np.sum(centers * centers, axis=1)[None, :]
        - 2.0 * points @ centers.T,
        0.0,
    )


def kmeans_centers(
    points: np.ndarray,
    count: int,
    random_seed: int,
    restarts: int = 5,
    maximum_iterations: int = 100,
) -> np.ndarray:
    rng = np.random.default_rng(random_seed)
    best_centers: np.ndarray | None = None
    best_inertia = np.inf
    for _ in range(restarts):
        centers = np.empty((count, points.shape[1]), dtype=np.float64)
        centers[0] = points[rng.integers(len(points))]
        closest_distances = squared_distances(points, centers[:1])[:, 0]
        for center in range(1, count):
            total_distance = float(np.sum(closest_distances))
            if total_distance == 0.0:
                selected = int(rng.integers(len(points)))
            else:
                selected = int(rng.choice(len(points), p=closest_distances / total_distance))
            centers[center] = points[selected]
            closest_distances = np.minimum(
                closest_distances,
                squared_distances(points, centers[center:center + 1])[:, 0],
            )

        previous_assignments: np.ndarray | None = None
        for _ in range(maximum_iterations):
            distances = squared_distances(points, centers)
            assignments = np.argmin(distances, axis=1)
            if previous_assignments is not None and np.array_equal(assignments, previous_assignments):
                break
            previous_assignments = assignments
            closest_distances = distances[np.arange(len(points)), assignments]
            for center in range(count):
                members = points[assignments == center]
                centers[center] = (
                    np.mean(members, axis=0)
                    if len(members)
                    else points[int(np.argmax(closest_distances))]
                )

        inertia = float(np.sum(np.min(squared_distances(points, centers), axis=1)))
        if inertia < best_inertia:
            best_inertia = inertia
            best_centers = centers.copy()
    if best_centers is None:
        raise RuntimeError("K-means did not produce prototype centers")
    return best_centers


def prototypes_per_class_accuracies(
    run_dir: Path,
    counts: tuple[int, ...],
    random_seed: int,
) -> dict[int, float]:
    train_codes, train_labels, validation_codes, validation_labels = (
        load_normalized_codes(run_dir)
    )
    accuracies: dict[int, float] = {}
    for count in counts:
        prototypes = np.concatenate(
            [
                kmeans_centers(
                    train_codes[train_labels == label],
                    count=count,
                    random_seed=random_seed + 1000 * (count - 5) + label,
                )
                for label in range(10)
            ]
        )
        prototype_labels = np.repeat(np.arange(10), count)
        predictions = prototype_labels[
            np.argmin(squared_distances(validation_codes, prototypes), axis=1)
        ]
        accuracies[count] = sweep.evaluation.accuracy(predictions, validation_labels)
    return accuracies


def plot_classifier_comparison(summary: dict[str, float | int]) -> Path:
    values = (
        ("Linear ridge", 100.0 * float(summary["validation_ridge_zscore_mean"]), "#287271"),
        ("10 class-mean prototypes", 100.0 * float(summary["validation_ten_prototype_zscore_mean"]), "#5470a8"),
        ("5 prototypes/digit (50)", 100.0 * float(summary["validation_five_prototypes_per_class_zscore_mean"]), "#8a6ca8"),
        ("10 prototypes/digit (100)", 100.0 * float(summary["validation_ten_prototypes_per_class_zscore_mean"]), "#5f82b2"),
        ("20 prototypes/digit (200)", 100.0 * float(summary["validation_twenty_prototypes_per_class_zscore_mean"]), "#4c9575"),
        ("1-nearest neighbour", 100.0 * float(summary["validation_nearest_zscore_mean"]), "#e9a03b"),
        ("5-nearest neighbours", 100.0 * float(summary["validation_five_nearest_zscore_mean"]), "#b65c4a"),
    )
    width, height = 1050, 630
    image = Image.new("RGB", (width, height), "#f5f5f2")
    draw = ImageDraw.Draw(image)
    title_font = ImageFont.load_default(size=22)
    font = ImageFont.load_default(size=14)
    draw.text((30, 18), "Full MNIST latent-code classifiers", font=title_font, fill="#202020")
    plot_left, plot_right = 300, 980
    for tick in range(0, 101, 20):
        x = plot_left + tick / 100.0 * (plot_right - plot_left)
        draw.line((x, 70, x, height - 45), fill="#d5d5d0")
        draw.text((x - 8, 51), str(tick), font=font, fill="#404040")
    for index, (label, value, color) in enumerate(values):
        y = 98 + index * 68
        draw.text((25, y + 1), label, font=font, fill="#202020")
        x = plot_left + value / 100.0 * (plot_right - plot_left)
        draw.rectangle((plot_left, y, x, y + 18), fill=color)
        draw.text((x + 6, y + 1), f"{value:.1f}%", font=font, fill="#202020")
    draw.text((plot_left, height - 28), "test accuracy", font=font, fill="#404040")
    path = FULL_OUTPUT_ROOT / "full_dataset_classifier_comparison.png"
    image.save(path)
    return path


def plot_comparison(summary: dict[str, float | int]) -> Path:
    small_summary_path = (
        sweep.USER_DATA
        / "output"
        / "cvae_mnist_direct_vae_sweep"
        / "latent_gating_confirmation_summary.json"
    )
    with small_summary_path.open() as handle:
        small_summaries = json.load(handle)
    small = next(item for item in small_summaries if item["penalty"] == 0.005)
    rows = (
        (
            "1,000 train / 200 test",
            100.0 * small["validation_ridge_zscore_mean"],
            100.0 * small["validation_nearest_zscore_mean"],
            small["active_latent_count_mean"],
        ),
        (
            "60,000 train / 10,000 test",
            100.0 * float(summary["validation_ridge_zscore_mean"]),
            100.0 * float(summary["validation_nearest_zscore_mean"]),
            float(summary["active_latent_count_mean"]),
        ),
    )

    width, height = 1320, 330
    image = Image.new("RGB", (width, height), "#f5f5f2")
    draw = ImageDraw.Draw(image)
    title_font = ImageFont.load_default(size=22)
    font = ImageFont.load_default(size=14)
    draw.text((30, 18), "Constant latent gates: dataset-size comparison", font=title_font, fill="#202020")
    draw.rectangle((770, 22, 784, 34), fill="#287271")
    draw.text((790, 19), "Linear ridge", font=font, fill="#303030")
    draw.rectangle((920, 22, 934, 34), fill="#e9a03b")
    draw.text((940, 19), "Nearest neighbour", font=font, fill="#303030")
    draw.rectangle((1135, 22, 1149, 34), fill="#5470a8")
    draw.text((1155, 19), "Active gates", font=font, fill="#303030")

    accuracy_left, accuracy_right = 300, 980
    count_left, count_right = 1080, 1280
    for tick in range(0, 101, 20):
        x = accuracy_left + tick / 100.0 * (accuracy_right - accuracy_left)
        draw.line((x, 72, x, height - 45), fill="#d5d5d0")
        draw.text((x - 8, 53), str(tick), font=font, fill="#404040")
    for tick in range(0, 65, 16):
        x = count_left + tick / 64.0 * (count_right - count_left)
        draw.line((x, 72, x, height - 45), fill="#ddddda")
        draw.text((x - 8, 53), str(tick), font=font, fill="#404040")

    for index, (label, ridge, nearest, active) in enumerate(rows):
        y = 98 + index * 94
        draw.text((25, y + 22), label, font=font, fill="#202020")
        for offset, value, color in ((8, ridge, "#287271"), (35, nearest, "#e9a03b")):
            x = accuracy_left + value / 100.0 * (accuracy_right - accuracy_left)
            draw.rectangle((accuracy_left, y + offset, x, y + offset + 14), fill=color)
            draw.text((x + 5, y + offset - 1), f"{value:.1f}%", font=font, fill="#202020")
        active_x = count_left + active / 64.0 * (count_right - count_left)
        draw.rectangle((count_left, y + 21, active_x, y + 37), fill="#5470a8")
        draw.text((active_x + 5, y + 20), f"{active:.1f}", font=font, fill="#202020")

    draw.text((accuracy_left, height - 28), "classification accuracy", font=font, fill="#404040")
    draw.text((count_left, height - 28), "active gates", font=font, fill="#404040")
    path = FULL_OUTPUT_ROOT / "full_dataset_comparison.png"
    image.save(path)
    return path


def main() -> None:
    args = parse_args()
    prepare_dataset()
    configure_sweep()
    FULL_OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    experiment = condition()
    results = []
    for replicate in range(1, args.replicates + 1):
        seed = args.seed_base + replicate
        result = sweep.run_condition(
            "full_dataset",
            experiment,
            replicate,
            seed,
            args,
        )
        if "validation_ten_prototype_zscore" not in result:
            run_id = f"full_dataset_{experiment.name}_r{replicate}_s{seed}_{args.ticks}"
            run_dir = FULL_OUTPUT_ROOT / run_id
            result["validation_ten_prototype_zscore"] = ten_prototype_accuracy(run_dir)
            with (run_dir / "result.json").open("w") as handle:
                json.dump(result, handle, indent=2, sort_keys=True)
                handle.write("\n")
        if "validation_five_nearest_zscore" not in result:
            run_id = f"full_dataset_{experiment.name}_r{replicate}_s{seed}_{args.ticks}"
            run_dir = FULL_OUTPUT_ROOT / run_id
            result["validation_five_nearest_zscore"] = five_nearest_neighbour_accuracy(run_dir)
            with (run_dir / "result.json").open("w") as handle:
                json.dump(result, handle, indent=2, sort_keys=True)
                handle.write("\n")
        prototype_metrics = {
            5: "validation_five_prototypes_per_class_zscore",
            10: "validation_ten_prototypes_per_class_zscore",
            20: "validation_twenty_prototypes_per_class_zscore",
        }
        missing_prototype_counts = tuple(
            count for count, metric in prototype_metrics.items() if metric not in result
        )
        if missing_prototype_counts:
            run_id = f"full_dataset_{experiment.name}_r{replicate}_s{seed}_{args.ticks}"
            run_dir = FULL_OUTPUT_ROOT / run_id
            accuracies = prototypes_per_class_accuracies(
                run_dir,
                counts=missing_prototype_counts,
                random_seed=seed + 100_000,
            )
            for count, accuracy in accuracies.items():
                result[prototype_metrics[count]] = accuracy
            with (run_dir / "result.json").open("w") as handle:
                json.dump(result, handle, indent=2, sort_keys=True)
                handle.write("\n")
        results.append(result)
    sweep.write_results("full_dataset_constant_gate.csv", results)
    metrics = (
        "validation_ridge_zscore",
        "validation_nearest_zscore",
        "validation_ten_prototype_zscore",
        "validation_five_nearest_zscore",
        "validation_five_prototypes_per_class_zscore",
        "validation_ten_prototypes_per_class_zscore",
        "validation_twenty_prototypes_per_class_zscore",
        "validation_absolute_reconstruction_error",
        "validation_kl_loss",
        "code_effective_rank",
        "active_latent_count",
        "expected_active_latent_count",
    )
    summary = {"runs": len(results), "ticks": args.ticks}
    for metric in metrics:
        values = np.asarray([result[metric] for result in results], dtype=np.float64)
        summary[metric + "_mean"] = float(np.mean(values))
        summary[metric + "_stddev"] = (
            float(np.std(values, ddof=1)) if len(values) > 1 else 0.0
        )
    with (FULL_OUTPUT_ROOT / "full_dataset_constant_gate_summary.json").open("w") as handle:
        json.dump(summary, handle, indent=2, sort_keys=True)
        handle.write("\n")
    plot_path = plot_comparison(summary)
    classifier_plot_path = plot_classifier_comparison(summary)
    print(json.dumps(summary, indent=2, sort_keys=True), flush=True)
    print(f"Plot: {plot_path}", flush=True)
    print(f"Classifier plot: {classifier_plot_path}", flush=True)


if __name__ == "__main__":
    main()
