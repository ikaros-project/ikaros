#!/usr/bin/env python3

"""Compare a requested direct-VAE latent size with the confirmed 10-D results."""

from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
from typing import Any

import numpy as np

import run_mnist_direct_vae_sweep as sweep


OUTPUT_ROOT = sweep.OUTPUT_ROOT
TEN_DIMENSIONAL_RESULTS = OUTPUT_ROOT / "confirmation.csv"

MATCHED_TEN_DIMENSIONAL_NAMES = {
    "beta_3e2": "refine_beta_3e2",
    "mse_linear": "mse_linear",
}

METRICS = (
    "validation_ridge_zscore",
    "validation_nearest_zscore",
    "validation_absolute_reconstruction_error",
    "validation_kl_loss",
    "code_effective_rank",
    "code_mean_absolute_correlation",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--latent-size", type=int, required=True)
    parser.add_argument("--ticks", type=int, default=50_000)
    parser.add_argument("--replicates", type=int, default=5)
    parser.add_argument("--seed-base", type=int, default=69_000)
    parser.add_argument("--agent", required=True)
    parser.add_argument("--print-tick-interval", type=int, default=10_000)
    parser.add_argument("--resume", action="store_true")
    return parser.parse_args()


def conditions_for_size(latent_size: int) -> tuple[sweep.Condition, ...]:
    if latent_size < 1:
        raise ValueError("latent size must be positive")
    return (
        sweep.Condition(
            f"latent{latent_size}_beta_3e2",
            "latent_size",
            f"{latent_size}-D, Bernoulli reconstruction, beta 0.03",
            {"latent_size": str(latent_size), "beta": "0.03"},
        ),
        sweep.Condition(
            f"latent{latent_size}_mse_linear",
            "latent_size",
            f"{latent_size}-D, linear mean-squared reconstruction, beta 0.0001",
            {
                "latent_size": str(latent_size),
                "reconstruction_loss": "mse",
                "output_activation": "linear",
            },
        ),
    )


def objective_name(condition_name: str) -> str:
    return "beta_3e2" if condition_name.endswith("beta_3e2") else "mse_linear"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as handle:
        return list(csv.DictReader(handle))


def matched_comparison(
    results: list[dict[str, Any]],
    ten_results: list[dict[str, str]],
    conditions: tuple[sweep.Condition, ...],
    latent_size: int,
) -> list[dict[str, Any]]:
    comparisons = []
    for condition in conditions:
        target_group = [result for result in results if result["name"] == condition.name]
        ten_name = MATCHED_TEN_DIMENSIONAL_NAMES[objective_name(condition.name)]
        ten_group = [result for result in ten_results if result["name"] == ten_name]
        ten_by_seed = {int(result["seed"]): result for result in ten_group}
        for metric in METRICS:
            ten_values = np.asarray(
                [float(ten_by_seed[result["seed"]][metric]) for result in target_group],
                dtype=np.float64,
            )
            target_values = np.asarray(
                [float(result[metric]) for result in target_group], dtype=np.float64
            )
            differences = target_values - ten_values
            comparisons.append(
                {
                    "setting": condition.name,
                    "metric": metric,
                    "target_latent_size": latent_size,
                    "latent10_mean": float(np.mean(ten_values)),
                    "latent10_stddev": float(np.std(ten_values, ddof=1)),
                    "target_mean": float(np.mean(target_values)),
                    "target_stddev": float(np.std(target_values, ddof=1)),
                    "paired_difference_mean": float(np.mean(differences)),
                    "paired_difference_standard_error": float(
                        np.std(differences, ddof=1) / math.sqrt(len(differences))
                    ),
                }
            )
    basename = f"latent10_vs_latent{latent_size}"
    with (OUTPUT_ROOT / f"{basename}.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(comparisons[0]))
        writer.writeheader()
        writer.writerows(comparisons)
    with (OUTPUT_ROOT / f"{basename}.json").open("w") as handle:
        json.dump(comparisons, handle, indent=2, sort_keys=True)
        handle.write("\n")
    return comparisons


def matched_prior_size_comparison(
    results: list[dict[str, Any]],
    conditions: tuple[sweep.Condition, ...],
    reference_size: int,
    latent_size: int,
) -> list[dict[str, Any]]:
    reference_path = OUTPUT_ROOT / f"latent{reference_size}_confirmation.csv"
    if not reference_path.exists():
        return []
    reference_results = read_csv(reference_path)
    comparisons = []
    for condition in conditions:
        objective = objective_name(condition.name)
        target_group = [result for result in results if result["name"] == condition.name]
        reference_name = f"latent{reference_size}_{objective}"
        reference_group = [
            result for result in reference_results if result["name"] == reference_name
        ]
        reference_by_seed = {int(result["seed"]): result for result in reference_group}
        for metric in METRICS:
            reference_values = np.asarray(
                [
                    float(reference_by_seed[result["seed"]][metric])
                    for result in target_group
                ],
                dtype=np.float64,
            )
            target_values = np.asarray(
                [float(result[metric]) for result in target_group], dtype=np.float64
            )
            differences = target_values - reference_values
            comparisons.append(
                {
                    "setting": condition.name,
                    "metric": metric,
                    "reference_latent_size": reference_size,
                    "target_latent_size": latent_size,
                    "reference_mean": float(np.mean(reference_values)),
                    "reference_stddev": float(np.std(reference_values, ddof=1)),
                    "target_mean": float(np.mean(target_values)),
                    "target_stddev": float(np.std(target_values, ddof=1)),
                    "paired_difference_mean": float(np.mean(differences)),
                    "paired_difference_standard_error": float(
                        np.std(differences, ddof=1) / math.sqrt(len(differences))
                    ),
                }
            )
    basename = f"latent{reference_size}_vs_latent{latent_size}"
    with (OUTPUT_ROOT / f"{basename}.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(comparisons[0]))
        writer.writeheader()
        writer.writerows(comparisons)
    with (OUTPUT_ROOT / f"{basename}.json").open("w") as handle:
        json.dump(comparisons, handle, indent=2, sort_keys=True)
        handle.write("\n")
    return comparisons


def comparison_plot_summaries(
    target_summaries: list[dict[str, Any]],
    ten_results: list[dict[str, str]],
    conditions: tuple[sweep.Condition, ...],
    latent_size: int,
) -> list[dict[str, Any]]:
    summaries = []
    intermediate = []
    intermediate_path = OUTPUT_ROOT / "latent20_summary.json"
    if latent_size != 20 and intermediate_path.exists():
        with intermediate_path.open() as handle:
            intermediate = json.load(handle)
    for condition in conditions:
        objective_key = objective_name(condition.name)
        ten_name = MATCHED_TEN_DIMENSIONAL_NAMES[objective_key]
        ten_group = [result for result in ten_results if result["name"] == ten_name]
        target = next(summary for summary in target_summaries if summary["name"] == condition.name)
        objective = "beta 0.03" if objective_key == "beta_3e2" else "linear MSE"
        ten_summary: dict[str, Any] = {"name": f"10-D {objective}"}
        for metric in ("validation_ridge_zscore", "validation_nearest_zscore"):
            values = np.asarray([float(result[metric]) for result in ten_group])
            ten_summary[metric + "_mean"] = float(np.mean(values))
            ten_summary[metric + "_stddev"] = float(np.std(values, ddof=1))
        summaries.append(ten_summary)
        if intermediate:
            intermediate_name = f"latent20_{objective_key}"
            intermediate_summary = dict(
                next(item for item in intermediate if item["name"] == intermediate_name)
            )
            intermediate_summary["name"] = f"20-D {objective}"
            summaries.append(intermediate_summary)
        target_summary = dict(target)
        target_summary["name"] = f"{latent_size}-D {objective}"
        summaries.append(target_summary)
    return summaries


def main() -> None:
    args = parse_args()
    if not TEN_DIMENSIONAL_RESULTS.exists():
        raise RuntimeError(
            "Run run_mnist_direct_vae_sweep.py first to create matched 10-D results"
        )
    conditions = conditions_for_size(args.latent_size)
    stage = f"latent{args.latent_size}"
    results = [
        sweep.run_condition(
            stage,
            condition,
            replicate,
            args.seed_base + replicate,
            args,
        )
        for condition in conditions
        for replicate in range(1, args.replicates + 1)
    ]
    sweep.write_results(f"{stage}_confirmation.csv", results)
    summaries = sweep.summarize_confirmation(results, f"{stage}_summary")
    ten_results = read_csv(TEN_DIMENSIONAL_RESULTS)
    comparisons = matched_comparison(results, ten_results, conditions, args.latent_size)
    prior_comparisons = matched_prior_size_comparison(
        results, conditions, 20, args.latent_size
    ) if args.latent_size != 20 else []
    sweep.plot_confirmation(
        comparison_plot_summaries(summaries, ten_results, conditions, args.latent_size),
        f"latent_sizes_through_{args.latent_size}.png",
        f"Direct VAE latent-size comparison through {args.latent_size} variables",
    )

    print("\nLatent-size comparison", flush=True)
    for summary in summaries:
        print(
            f"{summary['name']}: ridge={summary['validation_ridge_zscore_mean']:.1%} +/- "
            f"{summary['validation_ridge_zscore_stddev']:.1%}, "
            f"nearest={summary['validation_nearest_zscore_mean']:.1%} +/- "
            f"{summary['validation_nearest_zscore_stddev']:.1%}",
            flush=True,
        )
    for comparison in (*comparisons, *prior_comparisons):
        if comparison["metric"] not in (
            "validation_ridge_zscore",
            "validation_nearest_zscore",
        ):
            continue
        reference_size = comparison.get("reference_latent_size", 10)
        print(
            f"{comparison['setting']} {comparison['metric']}: paired "
            f"{args.latent_size}-D minus {reference_size}-D "
            f"{comparison['paired_difference_mean']:+.1%} +/- "
            f"{comparison['paired_difference_standard_error']:.1%} SE",
            flush=True,
        )


if __name__ == "__main__":
    main()
