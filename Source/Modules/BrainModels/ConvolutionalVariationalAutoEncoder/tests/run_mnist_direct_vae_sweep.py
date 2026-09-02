#!/usr/bin/env python3

"""Systematically compare learning settings for the direct 1024-10-1024 VAE."""

from __future__ import annotations

import argparse
import csv
import json
import math
import time
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import run_mnist_parameter_sweep as evaluation


REPOSITORY_ROOT = Path(__file__).resolve().parents[5]
USER_DATA = REPOSITORY_ROOT / "UserData"
DATA_ROOT = USER_DATA / "cvae_mnist_centered_32"
OUTPUT_ROOT = USER_DATA / "output" / "cvae_mnist_direct_vae_sweep"
CONSISTENCY_ROOT = OUTPUT_ROOT / "consistency_views"
IKAROS = REPOSITORY_ROOT / "Bin" / "ikaros"
TRAIN_TEMPLATE = Path(__file__).with_name("mnist_dense_vae_train.ikg")
EXTRACT_TEMPLATE = Path(__file__).with_name("mnist_dense_vae_extract.ikg")


BASE_PARAMETERS: dict[str, str] = {
    "feature_stage": "direct",
    "latent_mode": "dense",
    "latent_size": "10",
    "learning_rate": "0.001",
    "optimizer": "adam",
    "beta": "0.0001",
    "reconstruction_loss": "bernoulli",
    "sample": "yes",
    "reconstruction_source": "sample",
    "output_activation": "sigmoid",
    "latent_consistency_weight": "0",
    "latent_cluster_count": "1",
    "latent_cluster_temperature": "0.1",
    "latent_cluster_weight": "0",
    "latent_cluster_balance_weight": "0",
    "latent_cluster_balance_decay": "0.99",
    "latent_cluster_update": "gradient",
    "latent_cluster_commitment_weight": "0",
    "latent_decorrelation_weight": "0",
    "latent_decorrelation_decay": "0.99",
}


@dataclass(frozen=True)
class Condition:
    name: str
    family: str
    description: str
    overrides: dict[str, str] = field(default_factory=dict)
    paired_view: bool = False

    def parameters(self) -> dict[str, str]:
        return BASE_PARAMETERS | self.overrides


SCREEN_CONDITIONS = (
    Condition("baseline", "baseline", "Bernoulli, sampled latent, beta 1e-4, Adam 1e-3"),
    Condition(
        "mean_reconstruction",
        "sampling",
        "Use the latent mean during training",
        {"sample": "no", "reconstruction_source": "mean"},
    ),
    Condition("beta_0", "beta", "No Kullback-Leibler penalty", {"beta": "0"}),
    Condition("beta_1e5", "beta", "Beta 1e-5", {"beta": "0.00001"}),
    Condition("beta_1e3", "beta", "Beta 1e-3", {"beta": "0.001"}),
    Condition("beta_1e2", "beta", "Beta 1e-2", {"beta": "0.01"}),
    Condition("beta_1e1", "beta", "Beta 0.1", {"beta": "0.1"}),
    Condition("beta_1", "beta", "Beta 1", {"beta": "1"}),
    Condition(
        "mse_sigmoid",
        "reconstruction",
        "Mean squared error with sigmoid output",
        {"reconstruction_loss": "mse", "output_activation": "sigmoid"},
    ),
    Condition(
        "mse_linear",
        "reconstruction",
        "Mean squared error with linear output",
        {"reconstruction_loss": "mse", "output_activation": "linear"},
    ),
    Condition("adam_3e4", "optimizer", "Adam learning rate 3e-4", {"learning_rate": "0.0003"}),
    Condition("adam_3e3", "optimizer", "Adam learning rate 3e-3", {"learning_rate": "0.003"}),
    Condition(
        "sgd_1e3",
        "optimizer",
        "Stochastic gradient descent learning rate 1e-3",
        {"optimizer": "sgd", "learning_rate": "0.001"},
    ),
    Condition(
        "sgd_1e2",
        "optimizer",
        "Stochastic gradient descent learning rate 1e-2",
        {"optimizer": "sgd", "learning_rate": "0.01"},
    ),
    Condition("decor_1e3", "decorrelation", "Decorrelation weight 0.001", {"latent_decorrelation_weight": "0.001"}),
    Condition("decor_3e3", "decorrelation", "Decorrelation weight 0.003", {"latent_decorrelation_weight": "0.003"}),
    Condition("decor_1e2", "decorrelation", "Decorrelation weight 0.01", {"latent_decorrelation_weight": "0.01"}),
    Condition("decor_3e2", "decorrelation", "Decorrelation weight 0.03", {"latent_decorrelation_weight": "0.03"}),
    Condition("decor_1e1", "decorrelation", "Decorrelation weight 0.1", {"latent_decorrelation_weight": "0.1"}),
    Condition(
        "consistency_1e2",
        "consistency",
        "One-pixel paired-view consistency weight 0.01",
        {"latent_consistency_weight": "0.01"},
        True,
    ),
    Condition(
        "consistency_1e1",
        "consistency",
        "One-pixel paired-view consistency weight 0.1",
        {"latent_consistency_weight": "0.1"},
        True,
    ),
    Condition(
        "consistency_1",
        "consistency",
        "One-pixel paired-view consistency weight 1",
        {"latent_consistency_weight": "1"},
        True,
    ),
    Condition(
        "soft10_weak",
        "soft_prototypes",
        "Ten soft prototypes with weak attraction and balance",
        {
            "latent_cluster_count": "10",
            "latent_cluster_weight": "0.01",
            "latent_cluster_balance_weight": "0.1",
        },
    ),
    Condition(
        "soft10_medium",
        "soft_prototypes",
        "Ten soft prototypes with moderate attraction and balance",
        {
            "latent_cluster_count": "10",
            "latent_cluster_weight": "0.1",
            "latent_cluster_balance_weight": "1",
        },
    ),
    Condition(
        "soft10_strong",
        "soft_prototypes",
        "Ten soft prototypes with strong attraction and balance",
        {
            "latent_cluster_count": "10",
            "latent_cluster_weight": "1",
            "latent_cluster_balance_weight": "10",
        },
    ),
    Condition(
        "soft20_medium",
        "soft_prototypes",
        "Twenty soft prototypes with moderate attraction and balance",
        {
            "latent_cluster_count": "20",
            "latent_cluster_weight": "0.1",
            "latent_cluster_balance_weight": "1",
        },
    ),
    Condition(
        "vq10_weak",
        "vq_prototypes",
        "Ten vector-quantized prototypes with weak commitment",
        {
            "latent_cluster_count": "10",
            "latent_cluster_temperature": "0.03",
            "latent_cluster_weight": "0.01",
            "latent_cluster_balance_weight": "0.1",
            "latent_cluster_update": "vq",
            "latent_cluster_commitment_weight": "0.01",
        },
    ),
    Condition(
        "vq10_medium",
        "vq_prototypes",
        "Ten vector-quantized prototypes with moderate commitment",
        {
            "latent_cluster_count": "10",
            "latent_cluster_temperature": "0.03",
            "latent_cluster_weight": "0.1",
            "latent_cluster_balance_weight": "1",
            "latent_cluster_update": "vq",
            "latent_cluster_commitment_weight": "0.1",
        },
    ),
    Condition(
        "vq10_strong",
        "vq_prototypes",
        "Ten vector-quantized prototypes with strong commitment",
        {
            "latent_cluster_count": "10",
            "latent_cluster_temperature": "0.03",
            "latent_cluster_weight": "1",
            "latent_cluster_balance_weight": "10",
            "latent_cluster_update": "vq",
            "latent_cluster_commitment_weight": "1",
        },
    ),
    Condition(
        "vq20_medium",
        "vq_prototypes",
        "Twenty vector-quantized prototypes with moderate commitment",
        {
            "latent_cluster_count": "20",
            "latent_cluster_temperature": "0.03",
            "latent_cluster_weight": "0.1",
            "latent_cluster_balance_weight": "1",
            "latent_cluster_update": "vq",
            "latent_cluster_commitment_weight": "0.1",
        },
    ),
)


REFINEMENT_CONDITIONS = (
    Condition("refine_beta_3e3", "refinement", "Beta 0.003", {"beta": "0.003"}),
    Condition("refine_beta_3e2", "refinement", "Beta 0.03", {"beta": "0.03"}),
    Condition("refine_adam_6e4", "refinement", "Adam learning rate 6e-4", {"learning_rate": "0.0006"}),
    Condition(
        "pair_beta1e2_mean",
        "refinement",
        "Beta 0.01 with latent-mean reconstruction",
        {"beta": "0.01", "sample": "no", "reconstruction_source": "mean"},
    ),
    Condition(
        "pair_beta1e2_mse_linear",
        "refinement",
        "Beta 0.01 with linear mean-squared reconstruction",
        {"beta": "0.01", "reconstruction_loss": "mse", "output_activation": "linear"},
    ),
    Condition(
        "pair_mean_mse_linear",
        "refinement",
        "Latent-mean training with linear mean-squared reconstruction",
        {
            "sample": "no",
            "reconstruction_source": "mean",
            "reconstruction_loss": "mse",
            "output_activation": "linear",
        },
    ),
    Condition(
        "triple_beta_mean_mse_linear",
        "refinement",
        "Beta 0.01 and latent-mean training with linear mean-squared reconstruction",
        {
            "beta": "0.01",
            "sample": "no",
            "reconstruction_source": "mean",
            "reconstruction_loss": "mse",
            "output_activation": "linear",
        },
    ),
    Condition(
        "pair_beta1e2_vq10",
        "refinement",
        "Beta 0.01 with moderate ten-prototype vector quantization",
        {
            "beta": "0.01",
            "latent_cluster_count": "10",
            "latent_cluster_temperature": "0.03",
            "latent_cluster_weight": "0.1",
            "latent_cluster_balance_weight": "1",
            "latent_cluster_update": "vq",
            "latent_cluster_commitment_weight": "0.1",
        },
    ),
    Condition(
        "pair_beta1e2_consistency",
        "refinement",
        "Beta 0.01 with light one-pixel consistency",
        {"beta": "0.01", "latent_consistency_weight": "0.01"},
        True,
    ),
    Condition(
        "pair_beta1e2_decor",
        "refinement",
        "Beta 0.01 with decorrelation weight 0.03",
        {"beta": "0.01", "latent_decorrelation_weight": "0.03"},
    ),
    Condition(
        "pair_mse_linear_vq10",
        "refinement",
        "Linear mean-squared reconstruction with moderate ten-prototype vector quantization",
        {
            "reconstruction_loss": "mse",
            "output_activation": "linear",
            "latent_cluster_count": "10",
            "latent_cluster_temperature": "0.03",
            "latent_cluster_weight": "0.1",
            "latent_cluster_balance_weight": "1",
            "latent_cluster_update": "vq",
            "latent_cluster_commitment_weight": "0.1",
        },
    ),
    Condition(
        "pair_mse_linear_consistency",
        "refinement",
        "Linear mean-squared reconstruction with light one-pixel consistency",
        {
            "reconstruction_loss": "mse",
            "output_activation": "linear",
            "latent_consistency_weight": "0.01",
        },
        True,
    ),
    Condition(
        "pair_mse_linear_decor",
        "refinement",
        "Linear mean-squared reconstruction with decorrelation weight 0.03",
        {
            "reconstruction_loss": "mse",
            "output_activation": "linear",
            "latent_decorrelation_weight": "0.03",
        },
    ),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=50_000)
    parser.add_argument("--confirm-replicates", type=int, default=5)
    parser.add_argument("--screen-seed", type=int, default=68_001)
    parser.add_argument("--confirm-seed-base", type=int, default=69_000)
    parser.add_argument("--agent", required=True)
    parser.add_argument("--print-tick-interval", type=int, default=10_000)
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--only", nargs="*", default=[])
    parser.add_argument("--stop-after-screen", action="store_true")
    parser.add_argument("--stop-after-combinations", action="store_true")
    return parser.parse_args()


def shift_image(image: np.ndarray, row_shift: int, column_shift: int) -> np.ndarray:
    shifted = np.zeros_like(image)
    source_row_begin = max(0, -row_shift)
    source_row_end = min(image.shape[0], image.shape[0] - row_shift)
    source_column_begin = max(0, -column_shift)
    source_column_end = min(image.shape[1], image.shape[1] - column_shift)
    target_row_begin = max(0, row_shift)
    target_column_begin = max(0, column_shift)
    shifted[
        target_row_begin:target_row_begin + source_row_end - source_row_begin,
        target_column_begin:target_column_begin + source_column_end - source_column_begin,
    ] = image[source_row_begin:source_row_end, source_column_begin:source_column_end]
    return shifted


def prepare_consistency_views() -> None:
    expected = [CONSISTENCY_ROOT / f"image_{index:05d}.pgm" for index in range(1000)]
    if all(path.exists() for path in expected):
        return
    CONSISTENCY_ROOT.mkdir(parents=True, exist_ok=True)
    shifts = ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, 1), (-1, 1), (1, -1))
    for index, target in enumerate(expected):
        source = DATA_ROOT / "train" / f"image_{index:05d}.pgm"
        image = np.asarray(Image.open(source).convert("L"), dtype=np.uint8)
        shifted = shift_image(image, *shifts[index % len(shifts)])
        Image.fromarray(shifted, mode="L").save(target)


def set_parameters(module: ET.Element, condition: Condition, seed: int, training: bool) -> None:
    parameters = condition.parameters()
    for name, value in parameters.items():
        module.set(name, value)
    module.set("random_seed", str(seed))
    module.set("train", "yes" if training else "no")
    if not training:
        module.set("sample", "no")
        module.set("reconstruction_source", "mean")


def add_training_consistency(root: ET.Element) -> None:
    module = ET.Element(
        "module",
        {
            "class": "InputImage",
            "name": "Consistency",
            "_x": "40",
            "_y": "360",
            "filename": "output/cvae_mnist_direct_vae_sweep/consistency_views/image_#####.pgm",
            "filecount": "1000",
            "iterations": "0",
            "read_once": "no",
        },
    )
    root.insert(1, module)
    root.append(
        ET.Element(
            "connection",
            {
                "source": "Consistency.INTENSITY",
                "target": "VAE.CONSISTENCY_INPUT",
                "delay": "1",
            },
        )
    )


def configure_model(
    template: Path,
    target: Path,
    condition: Condition,
    seed: int,
    run_id: str,
    training: bool,
    split: str = "train",
) -> None:
    tree = ET.parse(template)
    root = tree.getroot()
    modules = {module.get("name"): module for module in root.findall("module")}
    set_parameters(modules["VAE"], condition, seed, training)
    if training:
        for connection in list(root.findall("connection")):
            if connection.get("target", "").startswith("Metrics."):
                root.remove(connection)
        root.remove(modules["Metrics"])
        if condition.paired_view:
            add_training_consistency(root)
    else:
        count = 1000 if split == "train" else 200
        modules["MNIST"].set("filename", f"cvae_mnist_centered_32/{split}/image_#####.pgm")
        modules["MNIST"].set("filecount", str(count))
        modules["Labels"].set("filename", f"cvae_mnist_centered_32/{split}/labels.csv")
        output_name = "train_codes.csv" if split == "train" else "validation_codes.csv"
        modules["Codes"].set("directory", "output/cvae_mnist_direct_vae_sweep")
        modules["Codes"].set("filename", f"{run_id}/{output_name}")
        for connection in list(root.findall("connection")):
            if connection.get("source") == "VAE.OUTPUT":
                root.remove(connection)
        root.append(
            ET.Element(
                "connection",
                {
                    "source": "VAE.CLUSTER_ASSIGNMENT",
                    "target": "Codes.INPUT",
                    "label": "cluster_assignment",
                },
            )
        )
    ET.indent(tree, space="    ")
    tree.write(target, encoding="unicode")


def code_statistics(codes: np.ndarray) -> dict[str, float]:
    centered = codes - np.mean(codes, axis=0)
    covariance = centered.T @ centered / max(1, len(centered) - 1)
    eigenvalues = np.maximum(np.linalg.eigvalsh(covariance), 0.0)
    denominator = float(np.sum(eigenvalues * eigenvalues))
    effective_rank = float(np.sum(eigenvalues) ** 2 / denominator) if denominator else 0.0
    standard_deviations = np.std(codes, axis=0)
    valid = standard_deviations > 1.0e-12
    mean_absolute_correlation = 0.0
    if np.count_nonzero(valid) > 1:
        correlation = np.corrcoef(codes[:, valid], rowvar=False)
        off_diagonal = correlation[~np.eye(correlation.shape[0], dtype=bool)]
        mean_absolute_correlation = float(np.mean(np.abs(off_diagonal)))
    return {
        "code_stddev_mean": float(np.mean(standard_deviations)),
        "code_stddev_min": float(np.min(standard_deviations)),
        "code_effective_rank": effective_rank,
        "code_mean_absolute_correlation": mean_absolute_correlation,
    }


def evaluate_run(run_dir: Path) -> dict[str, Any]:
    train_rows, train_skip = evaluation.read_and_align_rows(
        run_dir / "train_codes.csv", DATA_ROOT / "train" / "labels.csv"
    )
    validation_rows, validation_skip = evaluation.read_and_align_rows(
        run_dir / "validation_codes.csv", DATA_ROOT / "test" / "labels.csv"
    )
    train_labels = evaluation.labels_from_rows(train_rows)
    validation_labels = evaluation.labels_from_rows(validation_rows)
    train_codes = evaluation.column_matrix(train_rows, "latent_mean")
    validation_codes = evaluation.column_matrix(validation_rows, "latent_mean")
    train_z, validation_z = evaluation.zscore(train_codes, validation_codes)
    cluster_accuracy = evaluation.cluster_majority_accuracy(
        train_rows, validation_rows, train_labels, validation_labels
    )
    result: dict[str, Any] = {
        "train_samples": len(train_rows),
        "validation_samples": len(validation_rows),
        "train_alignment_skip": train_skip,
        "validation_alignment_skip": validation_skip,
        "validation_ridge_raw": evaluation.accuracy(
            evaluation.ridge_predictions(train_codes, train_labels, validation_codes),
            validation_labels,
        ),
        "validation_ridge_zscore": evaluation.accuracy(
            evaluation.ridge_predictions(train_z, train_labels, validation_z),
            validation_labels,
        ),
        "validation_nearest_raw": evaluation.accuracy(
            evaluation.nearest_predictions(train_codes, train_labels, validation_codes),
            validation_labels,
        ),
        "validation_nearest_zscore": evaluation.accuracy(
            evaluation.nearest_predictions(train_z, train_labels, validation_z),
            validation_labels,
        ),
        "validation_reconstruction_loss": evaluation.scalar_mean(
            validation_rows, "reconstruction_loss"
        ),
        "validation_absolute_reconstruction_error": evaluation.scalar_mean(
            validation_rows, "absolute_reconstruction_error"
        ),
        "validation_kl_loss": evaluation.scalar_mean(validation_rows, "kl_loss"),
        "validation_cluster_majority": cluster_accuracy,
    }
    result.update(code_statistics(train_codes))
    return result


def run_condition(
    stage: str,
    condition: Condition,
    replicate: int,
    seed: int,
    args: argparse.Namespace,
) -> dict[str, Any]:
    run_id = f"{stage}_{condition.name}_r{replicate}_s{seed}_{args.ticks}"
    run_dir = OUTPUT_ROOT / run_id
    result_path = run_dir / "result.json"
    if args.resume and result_path.exists():
        with result_path.open() as handle:
            return json.load(handle)

    run_dir.mkdir(parents=True, exist_ok=True)
    train_model = run_dir / "train.ikg"
    train_extract_model = run_dir / "extract_train.ikg"
    validation_extract_model = run_dir / "extract_validation.ikg"
    state_path = run_dir / "model.state"
    configure_model(TRAIN_TEMPLATE, train_model, condition, seed, run_id, True)
    configure_model(EXTRACT_TEMPLATE, train_extract_model, condition, seed, run_id, False, "train")
    configure_model(EXTRACT_TEMPLATE, validation_extract_model, condition, seed, run_id, False, "test")

    started = time.monotonic()
    print(f"[{stage}] {condition.name}: training seed {seed}", flush=True)
    evaluation.run_command(
        [
            str(IKAROS),
            "-b",
            "-s",
            str(args.ticks),
            "-P",
            str(args.print_tick_interval),
            "-A",
            args.agent,
            "-W",
            str(state_path),
            str(train_model),
        ],
        run_dir / "training.log",
    )
    extract_command = [str(IKAROS), "-b", "-A", args.agent, "-L", str(state_path)]
    evaluation.run_command(
        [*extract_command, "-s", "1005", str(train_extract_model)],
        run_dir / "extract_train.log",
    )
    evaluation.run_command(
        [*extract_command, "-s", "205", str(validation_extract_model)],
        run_dir / "extract_validation.log",
    )
    result = evaluate_run(run_dir)
    result.update(
        {
            "stage": stage,
            "name": condition.name,
            "family": condition.family,
            "description": condition.description,
            "parameters": condition.parameters(),
            "paired_view": condition.paired_view,
            "ticks": args.ticks,
            "replicate": replicate,
            "seed": seed,
            "elapsed_seconds": time.monotonic() - started,
        }
    )
    with result_path.open("w") as handle:
        json.dump(result, handle, indent=2, sort_keys=True)
        handle.write("\n")
    print(
        f"[{stage}] {condition.name}: ridge={result['validation_ridge_zscore']:.1%}, "
        f"nearest={result['validation_nearest_zscore']:.1%}, "
        f"MAE={result['validation_absolute_reconstruction_error']:.4f}",
        flush=True,
    )
    return result


def best_condition(
    results: list[dict[str, Any]], family: str, baseline: dict[str, Any]
) -> dict[str, Any]:
    candidates = [baseline, *(result for result in results if result["family"] == family)]
    return max(candidates, key=lambda result: result["validation_ridge_zscore"])


def combination_conditions(screen_results: list[dict[str, Any]]) -> tuple[Condition, ...]:
    baseline = next(result for result in screen_results if result["name"] == "baseline")
    core_families = ("sampling", "beta", "reconstruction", "optimizer")
    core_winners = [best_condition(screen_results, family, baseline) for family in core_families]
    core_overrides: dict[str, str] = {}
    core_names = []
    for winner in core_winners:
        if winner["name"] == "baseline":
            continue
        core_overrides.update(
            {
                key: value
                for key, value in winner["parameters"].items()
                if BASE_PARAMETERS.get(key) != value
            }
        )
        core_names.append(winner["name"])

    conditions = [
        Condition(
            "combined_core",
            "combination",
            "Best individually screened sampling, beta, reconstruction, and optimizer settings",
            core_overrides,
        )
    ]
    regularizer_families = (
        "decorrelation",
        "consistency",
        "soft_prototypes",
        "vq_prototypes",
    )
    regularizer_winners: dict[str, dict[str, Any]] = {}
    for family in regularizer_families:
        winner = best_condition(screen_results, family, baseline)
        if winner["name"] == "baseline":
            continue
        regularizer_winners[family] = winner
        overrides = core_overrides | {
            key: value
            for key, value in winner["parameters"].items()
            if BASE_PARAMETERS.get(key) != value
        }
        conditions.append(
            Condition(
                f"combined_{winner['name']}",
                "combination",
                f"Combined core plus {winner['description']}",
                overrides,
                winner["paired_view"],
            )
        )

    decorrelation = regularizer_winners.get("decorrelation")
    consistency = regularizer_winners.get("consistency")
    if decorrelation is not None and consistency is not None:
        overrides = dict(core_overrides)
        for winner in (decorrelation, consistency):
            overrides.update(
                {
                    key: value
                    for key, value in winner["parameters"].items()
                    if BASE_PARAMETERS.get(key) != value
                }
            )
        conditions.append(
            Condition(
                "combined_decor_consistency",
                "combination",
                "Combined core, decorrelation, and paired-view consistency",
                overrides,
                True,
            )
        )

    print("Core screen winners: " + ", ".join(core_names or ["baseline"]), flush=True)
    return (*conditions, *REFINEMENT_CONDITIONS)


def confirmation_conditions(
    all_results: list[dict[str, Any]], conditions: dict[str, Condition]
) -> tuple[Condition, ...]:
    ranked = sorted(all_results, key=lambda result: result["validation_ridge_zscore"], reverse=True)
    selected_names = ["baseline"]
    ridge_threshold = ranked[min(3, len(ranked) - 1)]["validation_ridge_zscore"]
    for result in ranked:
        if result["validation_ridge_zscore"] < ridge_threshold:
            break
        if result["name"] not in selected_names:
            selected_names.append(result["name"])
    nearest_winner = max(all_results, key=lambda result: result["validation_nearest_zscore"])
    if nearest_winner["name"] not in selected_names:
        selected_names.append(nearest_winner["name"])
    return tuple(conditions[name] for name in selected_names)


RESULT_COLUMNS = (
    "stage",
    "name",
    "family",
    "description",
    "ticks",
    "replicate",
    "seed",
    "validation_ridge_zscore",
    "validation_nearest_zscore",
    "validation_ridge_raw",
    "validation_nearest_raw",
    "validation_cluster_majority",
    "validation_reconstruction_loss",
    "validation_absolute_reconstruction_error",
    "validation_kl_loss",
    "code_stddev_mean",
    "code_stddev_min",
    "code_effective_rank",
    "code_mean_absolute_correlation",
    "elapsed_seconds",
)


def write_results(filename: str, results: list[dict[str, Any]]) -> None:
    with (OUTPUT_ROOT / filename).open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=RESULT_COLUMNS)
        writer.writeheader()
        for result in results:
            writer.writerow({key: result.get(key) for key in RESULT_COLUMNS})


def summarize_confirmation(
    results: list[dict[str, Any]], basename: str = "confirmation_summary"
) -> list[dict[str, Any]]:
    summaries = []
    for name in dict.fromkeys(result["name"] for result in results):
        group = [result for result in results if result["name"] == name]
        summary: dict[str, Any] = {
            "name": name,
            "description": group[0]["description"],
            "runs": len(group),
        }
        for metric in (
            "validation_ridge_zscore",
            "validation_nearest_zscore",
            "validation_reconstruction_loss",
            "validation_absolute_reconstruction_error",
            "validation_kl_loss",
            "code_effective_rank",
            "code_mean_absolute_correlation",
        ):
            values = np.asarray([result[metric] for result in group], dtype=np.float64)
            summary[metric + "_mean"] = float(np.mean(values))
            summary[metric + "_stddev"] = float(np.std(values, ddof=1)) if len(values) > 1 else 0.0
        summaries.append(summary)
    summaries.sort(key=lambda item: item["validation_ridge_zscore_mean"], reverse=True)
    with (OUTPUT_ROOT / f"{basename}.json").open("w") as handle:
        json.dump(summaries, handle, indent=2, sort_keys=True)
        handle.write("\n")
    with (OUTPUT_ROOT / f"{basename}.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(summaries[0]))
        writer.writeheader()
        writer.writerows(summaries)
    return summaries


def plot_screen(results: list[dict[str, Any]]) -> None:
    ranked = sorted(results, key=lambda result: result["validation_ridge_zscore"])
    row_height = 24
    width = 1250
    height = 90 + row_height * len(ranked)
    image = Image.new("RGB", (width, height), "#f5f5f2")
    draw = ImageDraw.Draw(image)
    title_font = ImageFont.load_default(size=22)
    font = ImageFont.load_default(size=13)
    draw.text((30, 18), "Direct 1024-10-1024 VAE screening", font=title_font, fill="#202020")
    chart_left, chart_right = 355, 1210
    draw.line((chart_left, 62, chart_left, height - 20), fill="#303030", width=2)
    for tick in range(0, 101, 10):
        x = chart_left + tick / 100 * (chart_right - chart_left)
        draw.line((x, 62, x, height - 20), fill="#d5d5d0")
        draw.text((x - 8, 43), str(tick), font=font, fill="#404040")
    for index, result in enumerate(ranked):
        y = 70 + index * row_height
        ridge = 100 * result["validation_ridge_zscore"]
        nearest = 100 * result["validation_nearest_zscore"]
        draw.text((20, y), result["name"], font=font, fill="#202020")
        ridge_x = chart_left + ridge / 100 * (chart_right - chart_left)
        nearest_x = chart_left + nearest / 100 * (chart_right - chart_left)
        draw.rectangle((chart_left, y + 2, ridge_x, y + 9), fill="#287271")
        draw.rectangle((chart_left, y + 11, nearest_x, y + 18), fill="#e9a03b")
        draw.text((ridge_x + 4, y), f"{ridge:.1f}", font=font, fill="#202020")
        draw.text((nearest_x + 4, y + 10), f"{nearest:.1f}", font=font, fill="#202020")
    image.save(OUTPUT_ROOT / "screening.png")


def plot_confirmation(
    summaries: list[dict[str, Any]],
    filename: str = "confirmation.png",
    title: str = "Matched-seed confirmation",
) -> None:
    width = 1250
    height = 170 + 90 * len(summaries)
    image = Image.new("RGB", (width, height), "#f5f5f2")
    draw = ImageDraw.Draw(image)
    title_font = ImageFont.load_default(size=22)
    font = ImageFont.load_default(size=14)
    draw.text((30, 18), title, font=title_font, fill="#202020")
    chart_left, chart_right = 355, 1210
    chart_top, chart_bottom = 80, height - 55
    for tick in range(0, 101, 10):
        x = chart_left + tick / 100 * (chart_right - chart_left)
        draw.line((x, chart_top, x, chart_bottom), fill="#d5d5d0")
        draw.text((x - 8, 58), str(tick), font=font, fill="#404040")
    for index, summary in enumerate(summaries):
        y = 100 + index * 90
        draw.text((20, y + 14), summary["name"], font=font, fill="#202020")
        for offset, metric, color in (
            (0, "validation_ridge_zscore", "#287271"),
            (28, "validation_nearest_zscore", "#e9a03b"),
        ):
            mean = 100 * summary[metric + "_mean"]
            error = 100 * summary[metric + "_stddev"]
            mean_x = chart_left + mean / 100 * (chart_right - chart_left)
            error_pixels = error / 100 * (chart_right - chart_left)
            draw.line((chart_left, y + offset + 8, mean_x, y + offset + 8), fill=color, width=12)
            draw.line((mean_x - error_pixels, y + offset + 8, mean_x + error_pixels, y + offset + 8), fill="#202020", width=2)
            draw.line((mean_x - error_pixels, y + offset + 3, mean_x - error_pixels, y + offset + 13), fill="#202020", width=2)
            draw.line((mean_x + error_pixels, y + offset + 3, mean_x + error_pixels, y + offset + 13), fill="#202020", width=2)
            draw.text(
                (mean_x + error_pixels + 8, y + offset),
                f"{mean:.1f} +/- {error:.1f}",
                font=font,
                fill="#202020",
            )
    draw.rectangle((420, height - 35, 438, height - 17), fill="#287271")
    draw.text((445, height - 37), "Linear ridge", font=font, fill="#202020")
    draw.rectangle((570, height - 35, 588, height - 17), fill="#e9a03b")
    draw.text((595, height - 37), "Nearest neighbour", font=font, fill="#202020")
    image.save(OUTPUT_ROOT / filename)


def main() -> None:
    args = parse_args()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    prepare_consistency_views()

    selected_screen = [
        condition
        for condition in SCREEN_CONDITIONS
        if not args.only or condition.name in args.only
    ]
    screen_results = [
        run_condition("screen", condition, 0, args.screen_seed, args)
        for condition in selected_screen
    ]
    write_results("screening.csv", screen_results)
    plot_screen(screen_results)
    if args.only or args.stop_after_screen:
        return

    combinations = combination_conditions(screen_results)
    combination_results = [
        run_condition("combination", condition, 0, args.screen_seed, args)
        for condition in combinations
    ]
    all_screen_results = screen_results + combination_results
    write_results("screening_and_combinations.csv", all_screen_results)
    plot_screen(all_screen_results)
    if args.stop_after_combinations:
        return

    condition_map = {condition.name: condition for condition in (*SCREEN_CONDITIONS, *combinations)}
    finalists = confirmation_conditions(all_screen_results, condition_map)
    with (OUTPUT_ROOT / "finalists.json").open("w") as handle:
        json.dump(
            [
                {
                    "name": condition.name,
                    "description": condition.description,
                    "parameters": condition.parameters(),
                    "paired_view": condition.paired_view,
                }
                for condition in finalists
            ],
            handle,
            indent=2,
            sort_keys=True,
        )
        handle.write("\n")

    confirmation_results = [
        run_condition(
            "confirm",
            condition,
            replicate,
            args.confirm_seed_base + replicate,
            args,
        )
        for condition in finalists
        for replicate in range(1, args.confirm_replicates + 1)
    ]
    write_results("confirmation.csv", confirmation_results)
    summaries = summarize_confirmation(confirmation_results)
    plot_confirmation(summaries)
    print("\nConfirmed ranking", flush=True)
    for summary in summaries:
        print(
            f"{summary['name']}: ridge={summary['validation_ridge_zscore_mean']:.1%} +/- "
            f"{summary['validation_ridge_zscore_stddev']:.1%}, "
            f"nearest={summary['validation_nearest_zscore_mean']:.1%} +/- "
            f"{summary['validation_nearest_zscore_stddev']:.1%}",
            flush=True,
        )


if __name__ == "__main__":
    main()
