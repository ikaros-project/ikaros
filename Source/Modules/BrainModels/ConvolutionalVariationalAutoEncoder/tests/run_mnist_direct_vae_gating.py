#!/usr/bin/env python3

"""Evaluate learned hard-concrete latent gates on the direct MNIST VAE."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from typing import Any

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import run_mnist_direct_vae_sweep as sweep


OUTPUT_ROOT = sweep.OUTPUT_ROOT
PENALTIES = (
    0.0,
    1.0e-4,
    3.0e-4,
    1.0e-3,
    3.0e-3,
    4.0e-3,
    5.0e-3,
    6.0e-3,
    8.0e-3,
    1.0e-2,
    3.0e-2,
    1.0e-1,
    3.0e-1,
)
SUMMARY_METRICS = (
    "validation_ridge_zscore",
    "validation_nearest_zscore",
    "validation_absolute_reconstruction_error",
    "validation_kl_loss",
    "code_effective_rank",
    "code_mean_absolute_correlation",
    "active_latent_count",
    "expected_active_latent_count",
    "latent_gate_mean",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=50_000)
    parser.add_argument("--replicates", type=int, default=5)
    parser.add_argument("--screen-seed", type=int, default=68_011)
    parser.add_argument("--seed-base", type=int, default=69_000)
    parser.add_argument("--agent", required=True)
    parser.add_argument("--print-tick-interval", type=int, default=10_000)
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--screen-only", action="store_true")
    return parser.parse_args()


def penalty_name(penalty: float) -> str:
    if penalty == 0.0:
        return "gate_penalty_0"
    exponent = int(np.floor(np.log10(penalty)))
    mantissa = int(round(penalty / (10.0 ** exponent)))
    return f"gate_penalty_{mantissa}e{abs(exponent)}"


def conditions() -> tuple[sweep.Condition, ...]:
    return tuple(
        sweep.Condition(
            penalty_name(penalty),
            "latent_gating",
            f"64-variable maximum with hard-concrete gate penalty {penalty:g}",
            {
                "latent_size": "64",
                "beta": "0.03",
                "latent_gating": "yes",
                "latent_gate_penalty": f"{penalty:g}",
            },
        )
        for penalty in PENALTIES
    )


def summarize(results: list[dict[str, Any]], basename: str) -> list[dict[str, Any]]:
    summaries: list[dict[str, Any]] = []
    for name in dict.fromkeys(result["name"] for result in results):
        group = [result for result in results if result["name"] == name]
        summary: dict[str, Any] = {
            "name": name,
            "description": group[0]["description"],
            "runs": len(group),
            "penalty": float(group[0]["parameters"]["latent_gate_penalty"]),
        }
        for metric in SUMMARY_METRICS:
            values = np.asarray([result[metric] for result in group], dtype=np.float64)
            summary[metric + "_mean"] = float(np.mean(values))
            summary[metric + "_stddev"] = (
                float(np.std(values, ddof=1)) if len(values) > 1 else 0.0
            )
        summaries.append(summary)
    summaries.sort(key=lambda item: item["penalty"])

    with (OUTPUT_ROOT / f"{basename}.json").open("w") as handle:
        json.dump(summaries, handle, indent=2, sort_keys=True)
        handle.write("\n")
    with (OUTPUT_ROOT / f"{basename}.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(summaries[0]))
        writer.writeheader()
        writer.writerows(summaries)
    return summaries


def plot_summaries(summaries: list[dict[str, Any]], filename: str, title: str) -> None:
    width = 1320
    row_height = 74
    height = 130 + row_height * len(summaries)
    image = Image.new("RGB", (width, height), "#f5f5f2")
    draw = ImageDraw.Draw(image)
    title_font = ImageFont.load_default(size=22)
    font = ImageFont.load_default(size=14)
    draw.text((30, 18), title, font=title_font, fill="#202020")
    for x, color, label in (
        (740, "#287271", "Linear ridge"),
        (880, "#e9a03b", "Nearest neighbour"),
        (1050, "#5470a8", "Thresholded"),
        (1170, "#9aa6bc", "Expected"),
    ):
        draw.rectangle((x, 22, x + 14, 34), fill=color)
        draw.text((x + 19, 19), label, font=font, fill="#303030")

    accuracy_left, accuracy_right = 310, 920
    count_left, count_right = 1040, 1280
    for tick in range(0, 101, 20):
        x = accuracy_left + tick / 100 * (accuracy_right - accuracy_left)
        draw.line((x, 65, x, height - 28), fill="#d5d5d0")
        draw.text((x - 8, 48), str(tick), font=font, fill="#404040")
    for tick in range(0, 65, 16):
        x = count_left + tick / 64 * (count_right - count_left)
        draw.line((x, 65, x, height - 28), fill="#ddddda")
        draw.text((x - 8, 48), str(tick), font=font, fill="#404040")
    draw.text((accuracy_left, height - 24), "classification accuracy (%)", font=font, fill="#404040")
    draw.text((count_left, height - 24), "active gates", font=font, fill="#404040")

    for index, summary in enumerate(summaries):
        y = 78 + index * row_height
        label = f"lambda={summary['penalty']:g}"
        draw.text((25, y + 18), label, font=font, fill="#202020")
        for offset, metric, color in (
            (6, "validation_ridge_zscore", "#287271"),
            (30, "validation_nearest_zscore", "#e9a03b"),
        ):
            mean = 100 * summary[metric + "_mean"]
            x = accuracy_left + mean / 100 * (accuracy_right - accuracy_left)
            draw.rectangle((accuracy_left, y + offset, x, y + offset + 12), fill=color)
            draw.text((x + 5, y + offset - 2), f"{mean:.1f}", font=font, fill="#202020")
        active = summary["active_latent_count_mean"]
        expected = summary["expected_active_latent_count_mean"]
        active_x = count_left + active / 64 * (count_right - count_left)
        expected_x = count_left + expected / 64 * (count_right - count_left)
        draw.rectangle((count_left, y + 8, active_x, y + 20), fill="#5470a8")
        draw.rectangle((count_left, y + 32, expected_x, y + 44), fill="#9aa6bc")
        draw.text((active_x + 4, y + 6), f"{active:.1f}", font=font, fill="#202020")
        draw.text((expected_x + 4, y + 30), f"{expected:.1f}", font=font, fill="#202020")
    image.save(OUTPUT_ROOT / filename)


def select_confirmation_conditions(
    screen_results: list[dict[str, Any]], all_conditions: tuple[sweep.Condition, ...]
) -> tuple[sweep.Condition, ...]:
    by_name = {condition.name: condition for condition in all_conditions}
    selected = [
        penalty_name(0.0),
        penalty_name(4.0e-3),
        penalty_name(5.0e-3),
        penalty_name(1.0e-2),
    ]
    selected.append(max(screen_results, key=lambda item: item["validation_ridge_zscore"])["name"])
    selected.append(max(screen_results, key=lambda item: item["validation_nearest_zscore"])["name"])

    middle = [
        item for item in screen_results
        if 24.0 <= item["active_latent_count"] <= 44.0
    ]
    if middle:
        selected.append(max(middle, key=lambda item: item["validation_ridge_zscore"])["name"])

    best_ridge = max(item["validation_ridge_zscore"] for item in screen_results)
    competitive = [
        item for item in screen_results
        if item["validation_ridge_zscore"] >= best_ridge - 0.02
    ]
    selected.append(min(competitive, key=lambda item: item["active_latent_count"])["name"])
    return tuple(by_name[name] for name in dict.fromkeys(selected))


def print_summaries(summaries: list[dict[str, Any]]) -> None:
    for summary in summaries:
        print(
            f"lambda={summary['penalty']:g}: "
            f"ridge={summary['validation_ridge_zscore_mean']:.1%}, "
            f"nearest={summary['validation_nearest_zscore_mean']:.1%}, "
            f"active={summary['active_latent_count_mean']:.1f}, "
            f"expected={summary['expected_active_latent_count_mean']:.1f}, "
            f"MAE={summary['validation_absolute_reconstruction_error_mean']:.4f}",
            flush=True,
        )


def main() -> None:
    args = parse_args()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    all_conditions = conditions()
    screen_results = [
        sweep.run_condition("gate_screen", condition, 0, args.screen_seed, args)
        for condition in all_conditions
    ]
    sweep.write_results("latent_gating_screen.csv", screen_results)
    screen_summaries = summarize(screen_results, "latent_gating_screen_summary")
    plot_summaries(
        screen_summaries,
        "latent_gating_screen.png",
        "Hard-concrete latent-gate penalty screen (64-variable maximum)",
    )
    print("\nGate penalty screen", flush=True)
    print_summaries(screen_summaries)
    if args.screen_only:
        return

    selected = select_confirmation_conditions(screen_results, all_conditions)
    print("Confirming: " + ", ".join(condition.name for condition in selected), flush=True)
    confirmation_results = [
        sweep.run_condition(
            "gate_confirmation",
            condition,
            replicate,
            args.seed_base + replicate,
            args,
        )
        for condition in selected
        for replicate in range(1, args.replicates + 1)
    ]
    sweep.write_results("latent_gating_confirmation.csv", confirmation_results)
    confirmation_summaries = summarize(
        confirmation_results, "latent_gating_confirmation_summary"
    )
    plot_summaries(
        confirmation_summaries,
        "latent_gating_confirmation.png",
        "Hard-concrete latent gates: matched-seed confirmation",
    )
    print("\nMatched-seed confirmation", flush=True)
    print_summaries(confirmation_summaries)


if __name__ == "__main__":
    main()
