#!/usr/bin/env python3

"""Compare fully downsampled five-level CVAE hierarchies on centered MNIST."""

from __future__ import annotations

import argparse
import csv
import gzip
import json
import os
import struct
import subprocess
import time
import xml.etree.ElementTree as ET
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np

import run_mnist_parameter_sweep as sweep


REPOSITORY_ROOT = Path(__file__).resolve().parents[5]
USER_DATA = REPOSITORY_ROOT / "UserData"
OUTPUT_ROOT = USER_DATA / "output" / "cvae_mnist_downsampled_five_layer"
DATA_ROOT = USER_DATA / "cvae_mnist_centered_32"
RAW_ROOT = USER_DATA / "cvae_mnist" / "raw"
IKAROS = REPOSITORY_ROOT / "Bin" / "ikaros"
TRAIN_TEMPLATE = Path(__file__).with_name("mnist_downsampled_five_layer_train.ikg")
EXTRACT_TEMPLATE = Path(__file__).with_name("mnist_downsampled_five_layer_extract.ikg")


@dataclass(frozen=True)
class Experiment:
    name: str
    kernel_size: int
    decorrelation_weight: float
    description: str


EXPERIMENTS = (
    Experiment("kernel3_plain", 3, 0.0, "3x3 kernels, selected plain latent-16 objective"),
    Experiment("kernel4_plain", 4, 0.0, "4x4 kernels, selected plain latent-16 objective"),
    Experiment(
        "kernel3_decor0p01",
        3,
        0.01,
        "3x3 kernels, latent-16 with top decorrelation weight 0.01",
    ),
    Experiment(
        "kernel4_decor0p01",
        4,
        0.01,
        "4x4 kernels, latent-16 with top decorrelation weight 0.01",
    ),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=50_000)
    parser.add_argument("--replicates", type=int, default=3)
    parser.add_argument("--seed-base", type=int, default=53_000)
    parser.add_argument("--agent", required=True)
    parser.add_argument("--only", nargs="*", default=[])
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--keep-going", action="store_true")
    parser.add_argument("--print-tick-interval", type=int, default=10_000)
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


def png_chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def write_png(path: Path, image: np.ndarray) -> None:
    scanlines = b"".join(b"\0" + row.tobytes() for row in image)
    header = struct.pack(">IIBBBBB", 32, 32, 8, 0, 0, 0, 0)
    with path.open("wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(png_chunk(b"IHDR", header))
        handle.write(png_chunk(b"IDAT", zlib.compress(scanlines)))
        handle.write(png_chunk(b"IEND", b""))


def prepare_split(split: str, count: int, image_archive: str, label_archive: str) -> None:
    target = DATA_ROOT / split
    completion_marker = target / ".complete_png"
    if completion_marker.exists():
        return
    target.mkdir(parents=True, exist_ok=True)
    images = read_idx_images(RAW_ROOT / image_archive, count)
    labels = read_idx_labels(RAW_ROOT / label_archive, count)
    for index, image in enumerate(images):
        write_png(target / f"image_{index:05d}.png", center_and_pad(image))
    with (target / "labels.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["label"])
        writer.writerows((int(label),) for label in labels)
    completion_marker.write_text(f"count={count}\n")


def prepare_dataset() -> None:
    prepare_split(
        "train",
        1000,
        "train-images-idx3-ubyte.gz",
        "train-labels-idx1-ubyte.gz",
    )
    prepare_split(
        "test",
        200,
        "t10k-images-idx3-ubyte.gz",
        "t10k-labels-idx1-ubyte.gz",
    )


def configure_model(
    template: Path,
    target: Path,
    experiment: Experiment,
    run_id: str,
    replicate_seed: int,
    training: bool,
    split: str = "train",
) -> None:
    tree = ET.parse(template)
    root = tree.getroot()
    modules = {module.get("name"): module for module in root.findall("module")}
    level_names = [
        "CVAE_Level1",
        "CVAE_Level2",
        "CVAE_Level3",
        "CVAE_Level4",
        "CVAE_Level5_TopCode",
    ]
    for offset, name in enumerate(level_names):
        module = modules[name]
        module.set("kernel_size", str(experiment.kernel_size))
        module.set("random_seed", str(replicate_seed + offset))
        module.set("train", "yes" if training else "no")
        if not training:
            module.set("sample", "no")
            module.set("reconstruction_source", "mean")
    modules["CVAE_Level5_TopCode"].set(
        "latent_decorrelation_weight", str(experiment.decorrelation_weight)
    )
    if training:
        modules["Metrics"].set("filename", f"{run_id}/metrics.csv")
    else:
        count = 1000 if split == "train" else 200
        output_split = "train" if split == "train" else "validation"
        modules["MNIST"].set(
            "filename", f"cvae_mnist_centered_32/{split}/image_#####.png"
        )
        modules["MNIST"].set("filecount", str(count))
        modules["Labels"].set("filename", f"cvae_mnist_centered_32/{split}/labels.csv")
        modules["Codes"].set("filename", f"{run_id}/{output_split}_codes.csv")
    ET.indent(tree, space="    ")
    tree.write(target, encoding="unicode")


def probe_metrics(
    train: np.ndarray,
    validation: np.ndarray,
    train_labels: np.ndarray,
    validation_labels: np.ndarray,
) -> dict[str, float]:
    train_z, validation_z = sweep.zscore(train, validation)
    return {
        "nearest": sweep.accuracy(
            sweep.nearest_predictions(train_z, train_labels, validation_z),
            validation_labels,
        ),
        "ridge": sweep.accuracy(
            sweep.ridge_predictions(train_z, train_labels, validation_z),
            validation_labels,
        ),
    }


def evaluate_run(run_dir: Path) -> dict[str, Any]:
    train_rows, train_skip = sweep.read_and_align_rows(
        run_dir / "train_codes.csv", DATA_ROOT / "train" / "labels.csv"
    )
    validation_rows, validation_skip = sweep.read_and_align_rows(
        run_dir / "validation_codes.csv", DATA_ROOT / "test" / "labels.csv"
    )
    train_labels = sweep.labels_from_rows(train_rows)
    validation_labels = sweep.labels_from_rows(validation_rows)
    train_top = sweep.column_matrix(train_rows, "top_code")
    validation_top = sweep.column_matrix(validation_rows, "top_code")
    train_level4 = sweep.column_matrix(train_rows, "level4_latent")
    validation_level4 = sweep.column_matrix(validation_rows, "level4_latent")
    top_metrics = probe_metrics(train_top, validation_top, train_labels, validation_labels)
    level4_metrics = probe_metrics(
        train_level4, validation_level4, train_labels, validation_labels
    )
    return {
        "train_samples": len(train_rows),
        "validation_samples": len(validation_rows),
        "train_alignment_skip": train_skip,
        "validation_alignment_skip": validation_skip,
        "top_code_dimension": int(train_top.shape[1]),
        "top_code_mean_stddev": float(np.mean(np.std(train_top, axis=0))),
        "validation_nearest_zscore": top_metrics["nearest"],
        "validation_ridge_zscore": top_metrics["ridge"],
        "level4_validation_nearest_zscore": level4_metrics["nearest"],
        "level4_validation_ridge_zscore": level4_metrics["ridge"],
        "validation_level1_absolute_reconstruction_error": sweep.scalar_mean(
            validation_rows, "level1_absolute_reconstruction_error"
        ),
        "validation_level4_reconstruction_loss": sweep.scalar_mean(
            validation_rows, "level4_reconstruction_loss"
        ),
        "validation_level5_reconstruction_loss": sweep.scalar_mean(
            validation_rows, "level5_reconstruction_loss"
        ),
        "training_tail": sweep.metrics_tail(run_dir / "metrics.csv"),
    }


def run_experiment(
    experiment: Experiment,
    ticks: int,
    replicate: int,
    seed_base: int,
    agent: str,
    print_interval: int,
) -> dict[str, Any]:
    replicate_seed = seed_base + 10 * replicate
    run_id = f"{experiment.name}_r{replicate}_s{replicate_seed}_{ticks}"
    run_dir = OUTPUT_ROOT / run_id
    run_dir.mkdir(parents=True, exist_ok=True)
    train_model = run_dir / "train.ikg"
    train_extract_model = run_dir / "extract_train.ikg"
    validation_extract_model = run_dir / "extract_validation.ikg"
    state_path = run_dir / "model.state"
    configure_model(
        TRAIN_TEMPLATE,
        train_model,
        experiment,
        run_id,
        replicate_seed,
        True,
    )
    configure_model(
        EXTRACT_TEMPLATE,
        train_extract_model,
        experiment,
        run_id,
        replicate_seed,
        False,
        "train",
    )
    configure_model(
        EXTRACT_TEMPLATE,
        validation_extract_model,
        experiment,
        run_id,
        replicate_seed,
        False,
        "test",
    )

    start = time.monotonic()
    print(f"\n[{run_id}] training: {experiment.description}", flush=True)
    sweep.run_command(
        [
            str(IKAROS),
            "-b",
            "-s",
            str(ticks),
            "-P",
            str(print_interval),
            "-A",
            agent,
            "-W",
            str(state_path),
            str(train_model),
        ],
        run_dir / "training.log",
    )
    common_extract = [str(IKAROS), "-b", "-A", agent, "-L", str(state_path)]
    print(f"[{run_id}] extracting training codes", flush=True)
    sweep.run_command(
        [*common_extract, "-s", "1005", str(train_extract_model)],
        run_dir / "extract_train.log",
    )
    print(f"[{run_id}] extracting validation codes", flush=True)
    sweep.run_command(
        [*common_extract, "-s", "205", str(validation_extract_model)],
        run_dir / "extract_validation.log",
    )
    result = evaluate_run(run_dir)
    result.update(
        {
            "name": experiment.name,
            "description": experiment.description,
            "kernel_size": experiment.kernel_size,
            "decorrelation_weight": experiment.decorrelation_weight,
            "ticks": ticks,
            "replicate": replicate,
            "seed": replicate_seed,
            "elapsed_seconds": time.monotonic() - start,
        }
    )
    with (run_dir / "result.json").open("w") as handle:
        json.dump(result, handle, indent=2, sort_keys=True)
        handle.write("\n")
    print(
        f"[{run_id}] ridge={result['validation_ridge_zscore']:.1%}, "
        f"nearest={result['validation_nearest_zscore']:.1%}, "
        f"L4 ridge={result['level4_validation_ridge_zscore']:.1%}, "
        f"MAE={result['validation_level1_absolute_reconstruction_error']:.6f}",
        flush=True,
    )
    return result


def load_results() -> list[dict[str, Any]]:
    results = []
    for path in OUTPUT_ROOT.glob("*/result.json"):
        with path.open() as handle:
            results.append(json.load(handle))
    return results


def write_aggregate(results: list[dict[str, Any]]) -> None:
    results.sort(key=lambda result: result["validation_ridge_zscore"], reverse=True)
    columns = [
        "name",
        "kernel_size",
        "decorrelation_weight",
        "ticks",
        "replicate",
        "seed",
        "validation_ridge_zscore",
        "validation_nearest_zscore",
        "level4_validation_ridge_zscore",
        "level4_validation_nearest_zscore",
        "top_code_mean_stddev",
        "validation_level1_absolute_reconstruction_error",
        "validation_level4_reconstruction_loss",
        "validation_level5_reconstruction_loss",
        "elapsed_seconds",
        "description",
    ]
    with (OUTPUT_ROOT / "results.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=columns)
        writer.writeheader()
        for result in results:
            writer.writerow({name: result.get(name) for name in columns})

    grouped: dict[tuple[str, int], list[dict[str, Any]]] = {}
    for result in results:
        grouped.setdefault((result["name"], result["ticks"]), []).append(result)
    summaries = []
    for (name, ticks), group in grouped.items():
        if len(group) < 2:
            continue
        summary: dict[str, Any] = {
            "name": name,
            "ticks": ticks,
            "runs": len(group),
            "kernel_size": group[0]["kernel_size"],
            "decorrelation_weight": group[0]["decorrelation_weight"],
            "description": group[0]["description"],
        }
        for metric in (
            "validation_ridge_zscore",
            "validation_nearest_zscore",
            "level4_validation_ridge_zscore",
            "level4_validation_nearest_zscore",
            "top_code_mean_stddev",
            "validation_level1_absolute_reconstruction_error",
        ):
            values = np.asarray([result[metric] for result in group])
            summary[metric + "_mean"] = float(np.mean(values))
            summary[metric + "_stddev"] = float(np.std(values, ddof=1))
        summaries.append(summary)
    summaries.sort(key=lambda result: result["validation_ridge_zscore_mean"], reverse=True)
    if summaries:
        with (OUTPUT_ROOT / "replicated_results.csv").open("w", newline="") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(summaries[0]))
            writer.writeheader()
            writer.writerows(summaries)

    try:
        matplotlib_dir = OUTPUT_ROOT / "matplotlib"
        matplotlib_dir.mkdir(exist_ok=True)
        os.environ.setdefault("MPLCONFIGDIR", str(matplotlib_dir))
        os.environ.setdefault("XDG_CACHE_HOME", str(matplotlib_dir))
        os.environ.setdefault("MPLBACKEND", "Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return
    plotted = [result for result in results if result["ticks"] >= 1000] or results
    positions = np.arange(len(plotted))
    width = 0.38
    _, axis = plt.subplots(figsize=(10, max(5.0, 0.45 * len(plotted) + 1.5)))
    axis.barh(
        positions - width / 2,
        [100.0 * result["validation_ridge_zscore"] for result in plotted],
        width,
        label="Top-code ridge probe",
    )
    axis.barh(
        positions + width / 2,
        [100.0 * result["validation_nearest_zscore"] for result in plotted],
        width,
        label="Top-code nearest neighbour",
    )
    axis.axvline(10.0, color="black", linewidth=1, linestyle="--", label="Chance")
    axis.set_yticks(
        positions,
        [f"{result['name']} r{result['replicate']} ({result['ticks'] // 1000}k)" for result in plotted],
    )
    axis.invert_yaxis()
    axis.set_xlabel("Held-out validation accuracy (%)")
    axis.set_title("Fully downsampled five-level CVAE frozen-code probes")
    axis.legend(loc="lower right")
    axis.grid(axis="x", alpha=0.25)
    plt.tight_layout()
    plt.savefig(OUTPUT_ROOT / "validation_accuracy.png", dpi=180)
    plt.close()

    if summaries:
        positions = np.arange(len(summaries))
        _, axis = plt.subplots(figsize=(10, max(4.5, 0.65 * len(summaries) + 1.5)))
        axis.barh(
            positions - width / 2,
            [100.0 * result["validation_ridge_zscore_mean"] for result in summaries],
            width,
            xerr=[100.0 * result["validation_ridge_zscore_stddev"] for result in summaries],
            capsize=3,
            label="Top-code ridge probe",
        )
        axis.barh(
            positions + width / 2,
            [100.0 * result["validation_nearest_zscore_mean"] for result in summaries],
            width,
            xerr=[100.0 * result["validation_nearest_zscore_stddev"] for result in summaries],
            capsize=3,
            label="Top-code nearest neighbour",
        )
        axis.axvline(10.0, color="black", linewidth=1, linestyle="--", label="Chance")
        axis.set_yticks(positions, [f"{result['name']} (n={result['runs']})" for result in summaries])
        axis.invert_yaxis()
        axis.set_xlabel("Held-out validation accuracy, mean +/- sample SD (%)")
        axis.set_title("Replicated fully downsampled five-level CVAE probes")
        axis.legend(loc="lower right")
        axis.grid(axis="x", alpha=0.25)
        plt.tight_layout()
        plt.savefig(OUTPUT_ROOT / "replicated_validation_accuracy.png", dpi=180)
        plt.close()


def main() -> int:
    args = parse_args()
    prepare_dataset()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    selected = list(EXPERIMENTS)
    if args.only:
        requested = set(args.only)
        selected = [experiment for experiment in selected if experiment.name in requested]
        missing = requested - {experiment.name for experiment in selected}
        if missing:
            raise RuntimeError(f"Unknown experiments: {', '.join(sorted(missing))}")

    failures = []
    for replicate in range(args.replicates):
        for experiment in selected:
            seed = args.seed_base + 10 * replicate
            result_path = (
                OUTPUT_ROOT
                / f"{experiment.name}_r{replicate}_s{seed}_{args.ticks}"
                / "result.json"
            )
            if args.resume and result_path.exists():
                print(f"[{experiment.name} r{replicate}] already complete", flush=True)
                continue
            try:
                run_experiment(
                    experiment,
                    args.ticks,
                    replicate,
                    args.seed_base,
                    args.agent,
                    args.print_tick_interval,
                )
            except Exception as error:
                failures.append(f"{experiment.name} r{replicate}: {error}")
                print(f"[{experiment.name} r{replicate}] FAILED: {error}", flush=True)
                if not args.keep_going:
                    break
        if failures and not args.keep_going:
            break
        write_aggregate(load_results())
    write_aggregate(load_results())
    if failures:
        print("Failures:\n" + "\n".join(failures))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
