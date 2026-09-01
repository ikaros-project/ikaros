#!/usr/bin/env python3

"""Run reproducible centered-MNIST CVAE screening experiments."""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import numpy as np


REPOSITORY_ROOT = Path(__file__).resolve().parents[5]
USER_DATA = REPOSITORY_ROOT / "UserData"
OUTPUT_ROOT = USER_DATA / "output" / "cvae_mnist_sweep"
IKAROS = REPOSITORY_ROOT / "Bin" / "ikaros"
TRAIN_MODEL = Path(__file__).with_name("mnist_parameter_sweep_train.ikg")
EXTRACT_MODEL = Path(__file__).with_name("mnist_parameter_sweep_extract.ikg")
TRAIN_LABELS = USER_DATA / "cvae_mnist_centered" / "train" / "labels.csv"
VALIDATION_LABELS = USER_DATA / "cvae_mnist_centered" / "test" / "labels.csv"

BASE_PARAMETERS: dict[str, str] = {
    "l1_latent_maps": "4",
    "l1_latent_kernel_size": "2",
    "l1_feature_maps": "20",
    "l1_kernel_size": "5",
    "l1_padding": "same",
    "l1_learning_rate": "0.001",
    "l1_beta": "0.0001",
    "l1_reconstruction_loss": "bernoulli",
    "l1_train_interval": "1",
    "l1_dense_train_interval": "2",
    "l1_reconstruction_source": "top_down",
    "l1_output_activation": "sigmoid",
    "l1_sample": "no",
    "l1_cluster_count": "1",
    "l1_cluster_temperature": "0.1",
    "l1_cluster_weight": "0",
    "l1_cluster_balance_weight": "0",
    "l1_cluster_update": "gradient",
    "l1_cluster_commitment_weight": "0",
    "l1_decorrelation_weight": "0",
    "top_latent_size": "32",
    "top_feature_maps": "12",
    "top_kernel_size": "5",
    "top_padding": "same",
    "top_learning_rate": "0.001",
    "top_beta": "0.0001",
    "top_reconstruction_loss": "mse",
    "top_train_interval": "1",
    "top_dense_train_interval": "1",
    "top_reconstruction_source": "mean",
    "top_output_activation": "linear",
    "top_sample": "no",
    "top_cluster_count": "1",
    "top_cluster_temperature": "0.1",
    "top_cluster_weight": "0",
    "top_cluster_balance_weight": "0",
    "top_cluster_update": "gradient",
    "top_cluster_commitment_weight": "0",
    "top_decorrelation_weight": "0",
}


@dataclass(frozen=True)
class Experiment:
    name: str
    description: str
    overrides: dict[str, str] = field(default_factory=dict)

    def parameters(self) -> dict[str, str]:
        return BASE_PARAMETERS | self.overrides


SCREENING_EXPERIMENTS = [
    Experiment("baseline", "Corrected clean two-layer baseline"),
    Experiment("top_beta_0", "No top-level Kullback-Leibler penalty", {"top_beta": "0"}),
    Experiment("top_beta_1e5", "Top-level beta 1e-5", {"top_beta": "0.00001"}),
    Experiment("top_beta_1e3", "Top-level beta 1e-3", {"top_beta": "0.001"}),
    Experiment("top_beta_1e2", "Top-level beta 1e-2", {"top_beta": "0.01"}),
    Experiment("latent_16", "16-dimensional top code", {"top_latent_size": "16"}),
    Experiment("latent_64", "64-dimensional top code", {"top_latent_size": "64"}),
    Experiment("latent_128", "128-dimensional top code", {"top_latent_size": "128"}),
    Experiment(
        "sample_both",
        "Sample from both latent distributions during training",
        {"l1_sample": "yes", "top_sample": "yes"},
    ),
    Experiment(
        "level1_mean_reconstruction",
        "Train Level 1 from its own latent mean without top-down reconstruction",
        {"l1_reconstruction_source": "mean"},
    ),
    Experiment(
        "top_decorrelation_0p01",
        "Top-code running decorrelation weight 0.01",
        {"top_decorrelation_weight": "0.01"},
    ),
    Experiment(
        "top_decorrelation_0p1",
        "Top-code running decorrelation weight 0.1",
        {"top_decorrelation_weight": "0.1"},
    ),
    Experiment(
        "prototype_soft",
        "Ten soft prototypes with moderate attraction and balance",
        {
            "top_cluster_count": "10",
            "top_cluster_temperature": "0.1",
            "top_cluster_weight": "0.1",
            "top_cluster_balance_weight": "1",
        },
    ),
    Experiment(
        "prototype_vq",
        "Ten vector-quantized prototypes with balanced usage",
        {
            "top_cluster_count": "10",
            "top_cluster_temperature": "0.03",
            "top_cluster_weight": "1",
            "top_cluster_balance_weight": "10",
            "top_cluster_update": "vq",
            "top_cluster_commitment_weight": "1",
        },
    ),
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=20_000)
    parser.add_argument("--agent", required=True)
    parser.add_argument("--only", nargs="*", default=[])
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--keep-going", action="store_true")
    parser.add_argument("--print-tick-interval", type=int, default=10_000)
    return parser.parse_args()


def run_command(command: list[str], log_path: Path) -> None:
    with log_path.open("w") as log:
        process = subprocess.Popen(
            command,
            cwd=REPOSITORY_ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        assert process.stdout is not None
        for line in process.stdout:
            log.write(line)
            if "tick" in line.lower() or "error" in line.lower():
                print(line.rstrip(), flush=True)
        return_code = process.wait()
    if return_code != 0:
        raise RuntimeError(f"Command failed with exit code {return_code}; see {log_path}")


PARAMETER_ATTRIBUTES = {
    "latent_maps": "latent_maps",
    "latent_kernel_size": "latent_kernel_size",
    "feature_maps": "feature_maps",
    "kernel_size": "kernel_size",
    "padding": "padding",
    "learning_rate": "learning_rate",
    "beta": "beta",
    "reconstruction_loss": "reconstruction_loss",
    "train_interval": "train_interval",
    "dense_train_interval": "dense_train_interval",
    "reconstruction_source": "reconstruction_source",
    "output_activation": "output_activation",
    "sample": "sample",
    "cluster_count": "latent_cluster_count",
    "cluster_temperature": "latent_cluster_temperature",
    "cluster_weight": "latent_cluster_weight",
    "cluster_balance_weight": "latent_cluster_balance_weight",
    "cluster_update": "latent_cluster_update",
    "cluster_commitment_weight": "latent_cluster_commitment_weight",
    "decorrelation_weight": "latent_decorrelation_weight",
    "latent_size": "latent_size",
}


def apply_module_parameters(
    module: ET.Element,
    prefix: str,
    parameters: dict[str, str],
    training: bool,
) -> None:
    for name, value in parameters.items():
        expected_prefix = prefix + "_"
        if not name.startswith(expected_prefix):
            continue
        short_name = name[len(expected_prefix):]
        attribute = PARAMETER_ATTRIBUTES.get(short_name)
        if attribute is not None:
            module.set(attribute, value)
    module.set("train", "yes" if training else "no")
    if not training:
        module.set("sample", "no")
        module.set("reconstruction_source", "mean")


def write_model(
    template: Path,
    target: Path,
    parameters: dict[str, str],
    training: bool,
    run_id: str,
    split: str = "train",
) -> None:
    tree = ET.parse(template)
    root = tree.getroot()
    modules = {module.get("name"): module for module in root.findall("module")}
    apply_module_parameters(modules["CVAE_Level1"], "l1", parameters, training)
    apply_module_parameters(modules["CVAE_TopCode"], "top", parameters, training)
    if training:
        modules["Metrics"].set("filename", f"{run_id}/metrics.csv")
    else:
        count = "1000" if split == "train" else "200"
        modules["MNIST"].set(
            "filename", f"cvae_mnist_centered/{split}/image_#####.png"
        )
        modules["MNIST"].set("filecount", count)
        modules["Labels"].set("filename", f"cvae_mnist_centered/{split}/labels.csv")
        output_split = "train" if split == "train" else "validation"
        modules["Codes"].set("filename", f"{run_id}/{output_split}_codes.csv")
    ET.indent(tree, space="    ")
    tree.write(target, encoding="unicode")


def read_expected_labels(path: Path) -> list[int]:
    with path.open(newline="") as handle:
        return [int(row["label"]) for row in csv.DictReader(handle)]


def read_and_align_rows(path: Path, expected_labels_path: Path) -> tuple[list[dict[str, str]], int]:
    expected = read_expected_labels(expected_labels_path)
    with path.open(newline="") as handle:
        rows = list(csv.DictReader(handle))
    if not rows:
        raise RuntimeError(f"{path} contains no data rows")

    best_skip = 0
    best_matches = -1
    best_compared = 0
    for skip in range(min(12, len(rows))):
        compared = min(100, len(rows) - skip, len(expected))
        matches = sum(
            int(round(float(rows[skip + index]["label"]))) == expected[index]
            for index in range(compared)
        )
        if matches > best_matches:
            best_skip = skip
            best_matches = matches
            best_compared = compared

    if best_compared == 0 or best_matches / best_compared < 0.98:
        raise RuntimeError(
            f"Could not align labels in {path}: best match was "
            f"{best_matches}/{best_compared} after skipping {best_skip} rows"
        )
    return rows[best_skip:best_skip + len(expected)], best_skip


def column_matrix(rows: list[dict[str, str]], prefix: str) -> np.ndarray:
    columns = [name for name in rows[0] if name.startswith(prefix + ":")]
    columns.sort(key=lambda name: int(name.rsplit(":", 1)[1]))
    if not columns and prefix in rows[0]:
        columns = [prefix]
    if not columns:
        raise RuntimeError(f"No columns found for {prefix}")
    values = np.asarray(
        [[float(row[column]) for column in columns] for row in rows],
        dtype=np.float64,
    )
    if not np.all(np.isfinite(values)):
        raise RuntimeError(f"Non-finite values found in {prefix}")
    return values


def labels_from_rows(rows: list[dict[str, str]]) -> np.ndarray:
    return np.asarray(
        [int(round(float(row["label"]))) for row in rows],
        dtype=np.int64,
    )


def zscore(train: np.ndarray, validation: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    mean = np.mean(train, axis=0)
    scale = np.std(train, axis=0)
    scale[scale < 1.0e-12] = 1.0
    return (train - mean) / scale, (validation - mean) / scale


def nearest_predictions(reference: np.ndarray, labels: np.ndarray, query: np.ndarray) -> np.ndarray:
    reference_squared = np.sum(reference * reference, axis=1)
    predictions: list[np.ndarray] = []
    for begin in range(0, len(query), 64):
        batch = query[begin:begin + 64]
        distances = (
            np.sum(batch * batch, axis=1, keepdims=True)
            + reference_squared[None, :]
            - 2.0 * batch @ reference.T
        )
        predictions.append(labels[np.argmin(distances, axis=1)])
    return np.concatenate(predictions)


def leave_one_out_nearest_accuracy(codes: np.ndarray, labels: np.ndarray) -> float:
    correct = 0
    for begin in range(0, len(codes), 64):
        batch = codes[begin:begin + 64]
        distances = (
            np.sum(batch * batch, axis=1, keepdims=True)
            + np.sum(codes * codes, axis=1)[None, :]
            - 2.0 * batch @ codes.T
        )
        row_count = len(batch)
        distances[np.arange(row_count), begin + np.arange(row_count)] = np.inf
        correct += int(np.sum(labels[np.argmin(distances, axis=1)] == labels[begin:begin + row_count]))
    return correct / len(codes)


def ridge_predictions(
    train: np.ndarray,
    labels: np.ndarray,
    validation: np.ndarray,
    regularization: float = 1.0e-3,
) -> np.ndarray:
    design = np.hstack([train, np.ones((len(train), 1), dtype=np.float64)])
    validation_design = np.hstack(
        [validation, np.ones((len(validation), 1), dtype=np.float64)]
    )
    targets = np.eye(10, dtype=np.float64)[labels]
    penalty = np.eye(design.shape[1], dtype=np.float64) * regularization
    penalty[-1, -1] = 0.0
    weights = np.linalg.solve(design.T @ design + penalty, design.T @ targets)
    return np.argmax(validation_design @ weights, axis=1)


def accuracy(predictions: np.ndarray, labels: np.ndarray) -> float:
    return float(np.mean(predictions == labels))


def scalar_mean(rows: list[dict[str, str]], name: str) -> float:
    column = name if name in rows[0] else f"{name}:0"
    values = [float(row[column]) for row in rows if math.isfinite(float(row[column]))]
    return float(np.mean(values))


def cluster_majority_accuracy(
    train_rows: list[dict[str, str]],
    validation_rows: list[dict[str, str]],
    train_labels: np.ndarray,
    validation_labels: np.ndarray,
) -> float | None:
    assignments = column_matrix(train_rows, "cluster_assignment")
    if assignments.shape[1] <= 1:
        return None
    validation_assignments = column_matrix(validation_rows, "cluster_assignment")
    winners = np.argmax(assignments, axis=1)
    mapping: dict[int, int] = {}
    for cluster in range(assignments.shape[1]):
        labels = train_labels[winners == cluster]
        if len(labels):
            mapping[cluster] = Counter(labels.tolist()).most_common(1)[0][0]
    default_label = Counter(train_labels.tolist()).most_common(1)[0][0]
    predictions = np.asarray(
        [mapping.get(int(cluster), default_label) for cluster in np.argmax(validation_assignments, axis=1)]
    )
    return accuracy(predictions, validation_labels)


def metrics_tail(path: Path, count: int = 1000) -> dict[str, float]:
    with path.open(newline="") as handle:
        rows = list(csv.DictReader(handle))[-count:]
    result: dict[str, float] = {}
    if not rows:
        return result
    for name in rows[0]:
        if name == "tick":
            continue
        values = [float(row[name]) for row in rows]
        if all(math.isfinite(value) for value in values):
            result[name] = float(np.mean(values))
    return result


def evaluate_run(run_dir: Path, parameters: dict[str, str]) -> dict[str, Any]:
    train_rows, train_skip = read_and_align_rows(run_dir / "train_codes.csv", TRAIN_LABELS)
    validation_rows, validation_skip = read_and_align_rows(
        run_dir / "validation_codes.csv", VALIDATION_LABELS
    )
    train_labels = labels_from_rows(train_rows)
    validation_labels = labels_from_rows(validation_rows)
    train_codes = column_matrix(train_rows, "top_code")
    validation_codes = column_matrix(validation_rows, "top_code")
    train_z, validation_z = zscore(train_codes, validation_codes)
    level1_train = column_matrix(train_rows, "level1_latent")
    level1_validation = column_matrix(validation_rows, "level1_latent")
    level1_train_z, level1_validation_z = zscore(level1_train, level1_validation)

    result: dict[str, Any] = {
        "parameters": parameters,
        "train_samples": len(train_rows),
        "validation_samples": len(validation_rows),
        "train_alignment_skip": train_skip,
        "validation_alignment_skip": validation_skip,
        "top_code_dimension": train_codes.shape[1],
        "top_code_mean_stddev": float(np.mean(np.std(train_codes, axis=0))),
        "top_code_min_stddev": float(np.min(np.std(train_codes, axis=0))),
        "top_code_max_stddev": float(np.max(np.std(train_codes, axis=0))),
        "train_leave_one_out_nearest_zscore": leave_one_out_nearest_accuracy(
            train_z, train_labels
        ),
        "validation_nearest_raw": accuracy(
            nearest_predictions(train_codes, train_labels, validation_codes),
            validation_labels,
        ),
        "validation_nearest_zscore": accuracy(
            nearest_predictions(train_z, train_labels, validation_z),
            validation_labels,
        ),
        "validation_ridge_raw": accuracy(
            ridge_predictions(train_codes, train_labels, validation_codes),
            validation_labels,
        ),
        "validation_ridge_zscore": accuracy(
            ridge_predictions(train_z, train_labels, validation_z),
            validation_labels,
        ),
        "level1_validation_nearest_zscore": accuracy(
            nearest_predictions(level1_train_z, train_labels, level1_validation_z),
            validation_labels,
        ),
        "level1_validation_ridge_zscore": accuracy(
            ridge_predictions(level1_train_z, train_labels, level1_validation_z),
            validation_labels,
        ),
        "validation_level1_reconstruction_loss": scalar_mean(
            validation_rows, "level1_reconstruction_loss"
        ),
        "validation_level1_absolute_reconstruction_error": scalar_mean(
            validation_rows, "level1_absolute_reconstruction_error"
        ),
        "validation_top_reconstruction_loss": scalar_mean(
            validation_rows, "top_code_reconstruction_loss"
        ),
        "training_tail": metrics_tail(run_dir / "metrics.csv"),
    }
    result["validation_cluster_majority"] = cluster_majority_accuracy(
        train_rows,
        validation_rows,
        train_labels,
        validation_labels,
    )
    return result


def run_experiment(experiment: Experiment, ticks: int, agent: str, print_interval: int) -> dict[str, Any]:
    run_id = f"{experiment.name}_{ticks}"
    run_dir = OUTPUT_ROOT / run_id
    run_dir.mkdir(parents=True, exist_ok=True)
    parameters = experiment.parameters()
    state_path = run_dir / "model.state"
    start = time.monotonic()
    train_model = run_dir / "train.ikg"
    extract_train_model = run_dir / "extract_train.ikg"
    extract_validation_model = run_dir / "extract_validation.ikg"
    write_model(TRAIN_MODEL, train_model, parameters, True, run_id)
    write_model(EXTRACT_MODEL, extract_train_model, parameters, False, run_id, "train")
    write_model(
        EXTRACT_MODEL,
        extract_validation_model,
        parameters,
        False,
        run_id,
        "test",
    )

    print(f"\n[{run_id}] training: {experiment.description}", flush=True)
    train_command = [
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
    ]
    run_command(train_command, run_dir / "training.log")

    common_extract = [
        str(IKAROS),
        "-b",
        "-A",
        agent,
        "-L",
        str(state_path),
    ]
    print(f"[{run_id}] extracting training codes", flush=True)
    run_command(
        [
            *common_extract,
            "-s",
            "1005",
            str(extract_train_model),
        ],
        run_dir / "extract_train.log",
    )
    print(f"[{run_id}] extracting validation codes", flush=True)
    run_command(
        [
            *common_extract,
            "-s",
            "205",
            str(extract_validation_model),
        ],
        run_dir / "extract_validation.log",
    )

    result = evaluate_run(run_dir, parameters)
    result.update(
        {
            "name": experiment.name,
            "description": experiment.description,
            "ticks": ticks,
            "elapsed_seconds": time.monotonic() - start,
        }
    )
    with (run_dir / "result.json").open("w") as handle:
        json.dump(result, handle, indent=2, sort_keys=True)
        handle.write("\n")
    print(
        f"[{run_id}] ridge={result['validation_ridge_zscore']:.1%}, "
        f"nearest={result['validation_nearest_zscore']:.1%}, "
        f"stddev={result['top_code_mean_stddev']:.4f}",
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
    results.sort(
        key=lambda result: (
            result["validation_ridge_zscore"],
            result["validation_nearest_zscore"],
        ),
        reverse=True,
    )
    columns = [
        "name",
        "ticks",
        "validation_ridge_zscore",
        "validation_nearest_zscore",
        "validation_ridge_raw",
        "validation_nearest_raw",
        "train_leave_one_out_nearest_zscore",
        "level1_validation_ridge_zscore",
        "level1_validation_nearest_zscore",
        "validation_cluster_majority",
        "top_code_mean_stddev",
        "validation_level1_reconstruction_loss",
        "validation_level1_absolute_reconstruction_error",
        "validation_top_reconstruction_loss",
        "elapsed_seconds",
        "description",
    ]
    with (OUTPUT_ROOT / "results.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=columns)
        writer.writeheader()
        for result in results:
            writer.writerow({name: result.get(name) for name in columns})

    try:
        matplotlib_dir = OUTPUT_ROOT / "matplotlib"
        matplotlib_dir.mkdir(exist_ok=True)
        os.environ.setdefault("MPLCONFIGDIR", str(matplotlib_dir))
        os.environ.setdefault("XDG_CACHE_HOME", str(matplotlib_dir))
        os.environ.setdefault("MPLBACKEND", "Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return
    names = [f"{result['name']} ({result['ticks'] // 1000}k)" for result in results]
    positions = np.arange(len(results))
    width = 0.38
    figure_height = max(5.0, 0.36 * len(results) + 1.5)
    _, axis = plt.subplots(figsize=(10, figure_height))
    axis.barh(
        positions - width / 2,
        [100.0 * result["validation_ridge_zscore"] for result in results],
        width,
        label="Linear ridge probe",
    )
    axis.barh(
        positions + width / 2,
        [100.0 * result["validation_nearest_zscore"] for result in results],
        width,
        label="Nearest neighbour",
    )
    axis.axvline(10.0, color="black", linewidth=1, linestyle="--", label="Chance")
    axis.set_yticks(positions, names)
    axis.invert_yaxis()
    axis.set_xlabel("Held-out validation accuracy (%)")
    axis.set_title("Centered-MNIST CVAE frozen-code probes")
    axis.legend(loc="lower right")
    axis.grid(axis="x", alpha=0.25)
    plt.tight_layout()
    plt.savefig(OUTPUT_ROOT / "validation_accuracy.png", dpi=180)
    plt.close()


def main() -> int:
    args = parse_args()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    selected = SCREENING_EXPERIMENTS
    if args.only:
        requested = set(args.only)
        selected = [experiment for experiment in selected if experiment.name in requested]
        missing = requested - {experiment.name for experiment in selected}
        if missing:
            raise RuntimeError(f"Unknown experiments: {', '.join(sorted(missing))}")

    failures: list[str] = []
    for experiment in selected:
        result_path = OUTPUT_ROOT / f"{experiment.name}_{args.ticks}" / "result.json"
        if args.resume and result_path.exists():
            print(f"[{experiment.name}] already complete; resuming past it", flush=True)
            continue
        try:
            run_experiment(experiment, args.ticks, args.agent, args.print_tick_interval)
        except Exception as error:
            failures.append(f"{experiment.name}: {error}")
            print(f"[{experiment.name}] FAILED: {error}", file=sys.stderr, flush=True)
            if not args.keep_going:
                break
        write_aggregate(load_results())

    results = load_results()
    write_aggregate(results)
    print(f"\nCompleted results: {len(results)}", flush=True)
    if results:
        best = max(results, key=lambda result: result["validation_ridge_zscore"])
        print(
            f"Best ridge result: {best['name']} at "
            f"{best['validation_ridge_zscore']:.1%} ({best['ticks']} ticks)",
            flush=True,
        )
    if failures:
        print("Failures:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
