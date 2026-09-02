#!/usr/bin/env python3

"""Compare constant and staged latent-gate training on the direct MNIST VAE."""

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
SUMMARY_METRICS = (
    "validation_ridge_zscore",
    "validation_nearest_zscore",
    "validation_absolute_reconstruction_error",
    "validation_kl_loss",
    "code_effective_rank",
    "code_mean_absolute_correlation",
    "active_latent_count",
    "expected_active_latent_count",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=50_000)
    parser.add_argument("--replicates", type=int, default=5)
    parser.add_argument("--seed-base", type=int, default=69_000)
    parser.add_argument("--agent", required=True)
    parser.add_argument("--print-tick-interval", type=int, default=10_000)
    parser.add_argument("--resume", action="store_true")
    return parser.parse_args()


def conditions(ticks: int) -> tuple[sweep.Condition, ...]:
    warmup = ticks // 5
    ramp = 2 * ticks // 5
    freeze = 4 * ticks // 5
    common = {
        "latent_size": "64",
        "beta": "0.03",
        "latent_gating": "yes",
        "latent_gate_penalty": "0.005",
    }
    return (
        sweep.Condition(
            "gate_constant_005",
            "gate_schedule",
            "Constant gate penalty 0.005",
            common,
        ),
        sweep.Condition(
            "gate_ramp_005",
            "gate_schedule",
            f"{warmup}-update warm-up and {ramp}-update penalty ramp",
            common
            | {
                "latent_gate_warmup_updates": str(warmup),
                "latent_gate_ramp_updates": str(ramp),
            },
        ),
        sweep.Condition(
            "gate_ramp_freeze_005",
            "gate_schedule",
            f"{warmup}-update warm-up, {ramp}-update ramp, and freeze at update {freeze}",
            common
            | {
                "latent_gate_warmup_updates": str(warmup),
                "latent_gate_ramp_updates": str(ramp),
                "latent_gate_freeze_update": str(freeze),
            },
        ),
    )


def summarize(results: list[dict[str, Any]]) -> list[dict[str, Any]]:
    summaries: list[dict[str, Any]] = []
    for name in dict.fromkeys(result["name"] for result in results):
        group = [result for result in results if result["name"] == name]
        summary: dict[str, Any] = {
            "name": name,
            "description": group[0]["description"],
            "runs": len(group),
        }
        for metric in SUMMARY_METRICS:
            values = np.asarray([result[metric] for result in group], dtype=np.float64)
            summary[metric + "_mean"] = float(np.mean(values))
            summary[metric + "_stddev"] = (
                float(np.std(values, ddof=1)) if len(values) > 1 else 0.0
            )
        summaries.append(summary)
    return summaries


def write_summaries(summaries: list[dict[str, Any]]) -> None:
    with (OUTPUT_ROOT / "latent_gate_schedule_summary.json").open("w") as handle:
        json.dump(summaries, handle, indent=2, sort_keys=True)
        handle.write("\n")
    with (OUTPUT_ROOT / "latent_gate_schedule_summary.csv").open(
        "w", newline=""
    ) as handle:
        writer = csv.DictWriter(handle, fieldnames=list(summaries[0]))
        writer.writeheader()
        writer.writerows(summaries)


def plot_summaries(summaries: list[dict[str, Any]]) -> Path:
    width, height = 1320, 410
    image = Image.new("RGB", (width, height), "#f5f5f2")
    draw = ImageDraw.Draw(image)
    title_font = ImageFont.load_default(size=22)
    font = ImageFont.load_default(size=14)
    draw.text(
        (30, 18),
        "Latent-gate schedule comparison (matched seeds)",
        font=title_font,
        fill="#202020",
    )
    draw.rectangle((760, 22, 774, 34), fill="#287271")
    draw.text((780, 19), "Linear ridge", font=font, fill="#303030")
    draw.rectangle((900, 22, 914, 34), fill="#e9a03b")
    draw.text((920, 19), "Nearest neighbour", font=font, fill="#303030")
    draw.rectangle((1110, 22, 1124, 34), fill="#5470a8")
    draw.text((1130, 19), "Active gates", font=font, fill="#303030")

    accuracy_left, accuracy_right = 390, 980
    count_left, count_right = 1080, 1280
    for tick in range(0, 101, 20):
        x = accuracy_left + tick / 100 * (accuracy_right - accuracy_left)
        draw.line((x, 72, x, height - 45), fill="#d5d5d0")
        draw.text((x - 8, 53), str(tick), font=font, fill="#404040")
    for tick in range(0, 65, 16):
        x = count_left + tick / 64 * (count_right - count_left)
        draw.line((x, 72, x, height - 45), fill="#ddddda")
        draw.text((x - 8, 53), str(tick), font=font, fill="#404040")

    display_labels = {
        "gate_constant_005": "Constant penalty",
        "gate_ramp_005": "Warm-up + ramp",
        "gate_ramp_freeze_005": "Warm-up + ramp + freeze",
    }
    for index, summary in enumerate(summaries):
        y = 92 + index * 88
        draw.text(
            (25, y + 21),
            display_labels[summary["name"]],
            font=font,
            fill="#202020",
        )
        for offset, metric, color in (
            (8, "validation_ridge_zscore", "#287271"),
            (34, "validation_nearest_zscore", "#e9a03b"),
        ):
            mean = 100.0 * summary[metric + "_mean"]
            x = accuracy_left + mean / 100.0 * (accuracy_right - accuracy_left)
            draw.rectangle((accuracy_left, y + offset, x, y + offset + 14), fill=color)
            draw.text((x + 5, y + offset - 1), f"{mean:.1f}%", font=font, fill="#202020")
        active = summary["active_latent_count_mean"]
        active_x = count_left + active / 64.0 * (count_right - count_left)
        draw.rectangle((count_left, y + 20, active_x, y + 36), fill="#5470a8")
        draw.text((active_x + 5, y + 19), f"{active:.1f}", font=font, fill="#202020")

    draw.text((accuracy_left, height - 28), "classification accuracy", font=font, fill="#404040")
    draw.text((count_left, height - 28), "active gates", font=font, fill="#404040")
    path = OUTPUT_ROOT / "latent_gate_schedule.png"
    image.save(path)
    return path


def main() -> None:
    args = parse_args()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    results = [
        sweep.run_condition(
            "gate_schedule",
            condition,
            replicate,
            args.seed_base + replicate,
            args,
        )
        for condition in conditions(args.ticks)
        for replicate in range(1, args.replicates + 1)
    ]
    sweep.write_results("latent_gate_schedule.csv", results)
    summaries = summarize(results)
    write_summaries(summaries)
    plot_path = plot_summaries(summaries)
    for summary in summaries:
        print(
            f"{summary['name']}: "
            f"ridge={summary['validation_ridge_zscore_mean']:.1%}, "
            f"nearest={summary['validation_nearest_zscore_mean']:.1%}, "
            f"active={summary['active_latent_count_mean']:.1f}, "
            f"MAE={summary['validation_absolute_reconstruction_error_mean']:.4f}",
            flush=True,
        )
    print(f"Plot: {plot_path}", flush=True)


if __name__ == "__main__":
    main()
