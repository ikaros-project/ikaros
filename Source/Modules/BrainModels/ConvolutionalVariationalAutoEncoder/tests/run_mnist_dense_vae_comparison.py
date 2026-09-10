#!/usr/bin/env python3

"""Compare convolution-free dense VAEs with 10- and 2-dimensional latent spaces."""

from __future__ import annotations

import argparse
import csv
import json
import time
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np

import run_mnist_parameter_sweep as sweep


REPOSITORY_ROOT = Path(__file__).resolve().parents[5]
USER_DATA = REPOSITORY_ROOT / "UserData"
DATA_ROOT = USER_DATA / "cvae_mnist_centered_32"
OUTPUT_ROOT = USER_DATA / "output" / "cvae_mnist_dense_vae"
IKAROS = REPOSITORY_ROOT / "Bin" / "ikaros"
TRAIN_TEMPLATE = Path(__file__).with_name("mnist_dense_vae_train.ikg")
EXTRACT_TEMPLATE = Path(__file__).with_name("mnist_dense_vae_extract.ikg")


@dataclass(frozen=True)
class Condition:
    name: str
    latent_size: int


CONDITIONS = (
    Condition("latent10", 10),
    Condition("latent2", 2),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=50_000)
    parser.add_argument("--replicates", type=int, default=3)
    parser.add_argument("--seed-base", type=int, default=67_000)
    parser.add_argument("--agent", required=True)
    parser.add_argument("--print-tick-interval", type=int, default=10_000)
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--reextract", action="store_true")
    return parser.parse_args()


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
    vae = modules["VAE"]
    vae.set("feature_stage", "direct")
    vae.set("latent_mode", "dense")
    vae.set("latent_size", str(condition.latent_size))
    vae.set("random_seed", str(seed))
    vae.set("train", "yes" if training else "no")
    vae.set("sample", "yes" if training else "no")
    vae.set("reconstruction_source", "sample" if training else "mean")
    if training:
        modules["Metrics"].set("filename", f"{run_id}/metrics.csv")
    else:
        count = 1000 if split == "train" else 200
        output_name = "train_codes.csv" if split == "train" else "validation_codes.csv"
        modules["MNIST"].set(
            "filename", f"cvae_mnist_centered_32/{split}/image_#####.pgm"
        )
        modules["MNIST"].set("filecount", str(count))
        modules["Labels"].set(
            "filename", f"cvae_mnist_centered_32/{split}/labels.csv"
        )
        modules["Codes"].set("filename", f"{run_id}/{output_name}")
    ET.indent(tree, space="    ")
    tree.write(target, encoding="unicode")


def evaluate_run(run_dir: Path) -> dict[str, Any]:
    train_rows, train_skip = sweep.read_and_align_rows(
        run_dir / "train_codes.csv", DATA_ROOT / "train" / "labels.csv"
    )
    validation_rows, validation_skip = sweep.read_and_align_rows(
        run_dir / "validation_codes.csv", DATA_ROOT / "test" / "labels.csv"
    )
    train_labels = sweep.labels_from_rows(train_rows)
    validation_labels = sweep.labels_from_rows(validation_rows)
    train_codes = sweep.column_matrix(train_rows, "latent_mean")
    validation_codes = sweep.column_matrix(validation_rows, "latent_mean")
    train_z, validation_z = sweep.zscore(train_codes, validation_codes)
    return {
        "train_samples": len(train_rows),
        "validation_samples": len(validation_rows),
        "train_alignment_skip": train_skip,
        "validation_alignment_skip": validation_skip,
        "code_dimension": int(train_codes.shape[1]),
        "code_mean_stddev": float(np.mean(np.std(train_codes, axis=0))),
        "code_min_stddev": float(np.min(np.std(train_codes, axis=0))),
        "validation_nearest_raw": sweep.accuracy(
            sweep.nearest_predictions(train_codes, train_labels, validation_codes),
            validation_labels,
        ),
        "validation_nearest_zscore": sweep.accuracy(
            sweep.nearest_predictions(train_z, train_labels, validation_z),
            validation_labels,
        ),
        "validation_ridge_raw": sweep.accuracy(
            sweep.ridge_predictions(train_codes, train_labels, validation_codes),
            validation_labels,
        ),
        "validation_ridge_zscore": sweep.accuracy(
            sweep.ridge_predictions(train_z, train_labels, validation_z),
            validation_labels,
        ),
        "validation_reconstruction_loss": sweep.scalar_mean(
            validation_rows, "reconstruction_loss"
        ),
        "validation_absolute_reconstruction_error": sweep.scalar_mean(
            validation_rows, "absolute_reconstruction_error"
        ),
        "validation_kl_loss": sweep.scalar_mean(validation_rows, "kl_loss"),
        "training_tail": sweep.metrics_tail(run_dir / "metrics.csv"),
    }


def run_condition(
    condition: Condition,
    replicate: int,
    args: argparse.Namespace,
) -> dict[str, Any]:
    seed = args.seed_base + replicate
    run_id = f"{condition.name}_r{replicate}_s{seed}_{args.ticks}"
    run_dir = OUTPUT_ROOT / run_id
    result_path = run_dir / "result.json"
    if args.resume and result_path.exists() and not args.reextract:
        with result_path.open() as handle:
            return json.load(handle)

    run_dir.mkdir(parents=True, exist_ok=True)
    train_model = run_dir / "train.ikg"
    train_extract_model = run_dir / "extract_train.ikg"
    validation_extract_model = run_dir / "extract_validation.ikg"
    state_path = run_dir / "model.state"
    configure_model(TRAIN_TEMPLATE, train_model, condition, seed, run_id, True)
    configure_model(
        EXTRACT_TEMPLATE,
        train_extract_model,
        condition,
        seed,
        run_id,
        False,
        "train",
    )
    configure_model(
        EXTRACT_TEMPLATE,
        validation_extract_model,
        condition,
        seed,
        run_id,
        False,
        "test",
    )

    started = time.monotonic()
    if args.reextract and state_path.exists():
        print(f"\n[{run_id}] reusing trained state", flush=True)
    else:
        print(f"\n[{run_id}] training {condition.latent_size}-D dense VAE", flush=True)
        sweep.run_command(
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
    common_extract = [str(IKAROS), "-b", "-A", args.agent, "-L", str(state_path)]
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
            "name": condition.name,
            "latent_size": condition.latent_size,
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
        f"[{run_id}] ridge={result['validation_ridge_zscore']:.1%}, "
        f"nearest={result['validation_nearest_zscore']:.1%}, "
        f"MAE={result['validation_absolute_reconstruction_error']:.4f}",
        flush=True,
    )
    return result


SUMMARY_METRICS = (
    "validation_ridge_zscore",
    "validation_nearest_zscore",
    "validation_reconstruction_loss",
    "validation_absolute_reconstruction_error",
    "validation_kl_loss",
    "code_mean_stddev",
)


def write_results(results: list[dict[str, Any]]) -> list[dict[str, Any]]:
    columns = [
        "name",
        "latent_size",
        "ticks",
        "replicate",
        "seed",
        *SUMMARY_METRICS,
        "validation_ridge_raw",
        "validation_nearest_raw",
        "elapsed_seconds",
    ]
    with (OUTPUT_ROOT / "results.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=columns)
        writer.writeheader()
        for result in results:
            writer.writerow({name: result.get(name) for name in columns})

    summaries = []
    for condition in CONDITIONS:
        group = [result for result in results if result["name"] == condition.name]
        summary: dict[str, Any] = {
            "name": condition.name,
            "latent_size": condition.latent_size,
            "runs": len(group),
            "ticks": group[0]["ticks"],
        }
        for metric in SUMMARY_METRICS:
            values = np.asarray([result[metric] for result in group], dtype=np.float64)
            summary[metric + "_mean"] = float(np.mean(values))
            summary[metric + "_stddev"] = float(np.std(values, ddof=1)) if len(values) > 1 else 0.0
        summaries.append(summary)

    summary_columns = list(summaries[0])
    with (OUTPUT_ROOT / "summary.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=summary_columns)
        writer.writeheader()
        writer.writerows(summaries)
    with (OUTPUT_ROOT / "summary.json").open("w") as handle:
        json.dump(summaries, handle, indent=2, sort_keys=True)
        handle.write("\n")
    return summaries


def aligned_data(run_dir: Path, split: str) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    label_path = DATA_ROOT / split / "labels.csv"
    filename = "train_codes.csv" if split == "train" else "validation_codes.csv"
    rows, _ = sweep.read_and_align_rows(run_dir / filename, label_path)
    return (
        sweep.column_matrix(rows, "latent_mean"),
        sweep.column_matrix(rows, "reconstruction"),
        sweep.labels_from_rows(rows),
    )


def representative_run(results: list[dict[str, Any]], condition: Condition) -> Path:
    result = min(
        (item for item in results if item["name"] == condition.name),
        key=lambda item: item["replicate"],
    )
    return OUTPUT_ROOT / f"{condition.name}_r{result['replicate']}_s{result['seed']}_{result['ticks']}"


def plot_results(results: list[dict[str, Any]], summaries: list[dict[str, Any]]) -> None:
    from PIL import Image, ImageDraw, ImageFont

    title_font = ImageFont.load_default(size=22)
    label_font = ImageFont.load_default(size=16)
    small_font = ImageFont.load_default(size=13)
    colors = [
        "#4e79a7", "#f28e2b", "#e15759", "#76b7b2", "#59a14f",
        "#edc948", "#b07aa1", "#ff9da7", "#9c755f", "#bab0ac",
    ]

    comparison = Image.new("RGB", (1200, 500), "#f5f5f2")
    draw = ImageDraw.Draw(comparison)
    panels = ((70, 65, 575, 425), (690, 65, 1150, 425))
    for x0, y0, x1, y1 in panels:
        draw.line((x0, y1, x1, y1), fill="#303030", width=2)
        draw.line((x0, y0, x0, y1), fill="#303030", width=2)
    draw.text((70, 18), "Dense VAE latent-size comparison", font=title_font, fill="#202020")
    draw.text((180, 42), "Information retained in latent means", font=label_font, fill="#202020")
    draw.text((790, 42), "Held-out reconstruction", font=label_font, fill="#202020")

    accuracy_metrics = (
        ("validation_ridge_zscore", "Ridge", "#287271"),
        ("validation_nearest_zscore", "Nearest", "#e9a03b"),
    )
    for tick in range(0, 101, 20):
        y = panels[0][3] - tick / 100 * (panels[0][3] - panels[0][1])
        draw.line((panels[0][0], y, panels[0][2], y), fill="#d7d7d2")
        draw.text((35, y - 7), str(tick), font=small_font, fill="#404040")
    group_centers = (235, 425)
    for group, (summary, center) in enumerate(zip(summaries, group_centers)):
        for offset, (metric, _, color) in enumerate(accuracy_metrics):
            mean = 100 * summary[metric + "_mean"]
            error = 100 * summary[metric + "_stddev"]
            x0 = center - 45 + offset * 48
            x1 = x0 + 38
            y = panels[0][3] - mean / 100 * (panels[0][3] - panels[0][1])
            draw.rectangle((x0, y, x1, panels[0][3]), fill=color)
            error_pixels = error / 100 * (panels[0][3] - panels[0][1])
            draw.line(((x0 + x1) / 2, y - error_pixels, (x0 + x1) / 2, y + error_pixels), fill="#202020", width=2)
            draw.text((x0 - 3, y - 20), f"{mean:.1f}", font=small_font, fill="#202020")
        draw.text((center - 25, 438), f"{summary['latent_size']}-D", font=label_font, fill="#202020")
    for index, (_, label, color) in enumerate(accuracy_metrics):
        x = 340 + index * 105
        draw.rectangle((x, 470, x + 16, 486), fill=color)
        draw.text((x + 22, 469), label, font=small_font, fill="#303030")
    draw.text((5, 225), "Accuracy (%)", font=small_font, fill="#303030")

    mae_max = max(item["validation_absolute_reconstruction_error_mean"] +
                  item["validation_absolute_reconstruction_error_stddev"] for item in summaries) * 1.25
    for tick in range(6):
        value = mae_max * tick / 5
        y = panels[1][3] - tick / 5 * (panels[1][3] - panels[1][1])
        draw.line((panels[1][0], y, panels[1][2], y), fill="#d7d7d2")
        draw.text((645, y - 7), f"{value:.2f}", font=small_font, fill="#404040")
    for summary, center, color in zip(summaries, (820, 1030), ("#287271", "#e76f51")):
        mean = summary["validation_absolute_reconstruction_error_mean"]
        error = summary["validation_absolute_reconstruction_error_stddev"]
        x0, x1 = center - 38, center + 38
        y = panels[1][3] - mean / mae_max * (panels[1][3] - panels[1][1])
        draw.rectangle((x0, y, x1, panels[1][3]), fill=color)
        error_pixels = error / mae_max * (panels[1][3] - panels[1][1])
        draw.line((center, y - error_pixels, center, y + error_pixels), fill="#202020", width=2)
        draw.text((center - 28, y - 20), f"{mean:.3f}", font=small_font, fill="#202020")
        draw.text((center - 25, 438), f"{summary['latent_size']}-D", font=label_font, fill="#202020")
    comparison.save(OUTPUT_ROOT / "comparison.png")

    latent_plot = Image.new("RGB", (1300, 620), "#f5f5f2")
    draw = ImageDraw.Draw(latent_plot)
    draw.text((55, 15), "Frozen validation latent means", font=title_font, fill="#202020")
    draw.text((55, 45), "Labels are shown only for evaluation", font=small_font, fill="#505050")
    plot_boxes = ((55, 90, 620, 545), (680, 90, 1245, 545))
    for box, condition in zip(plot_boxes, CONDITIONS):
        run_dir = representative_run(results, condition)
        train_codes, _, _ = aligned_data(run_dir, "train")
        validation_codes, _, validation_labels = aligned_data(run_dir, "test")
        train_z, validation_z = sweep.zscore(train_codes, validation_codes)
        if condition.latent_size > 2:
            _, _, components = np.linalg.svd(
                train_z - np.mean(train_z, axis=0), full_matrices=False
            )
            coordinates = validation_z @ components[:2].T
            subtitle = "first two principal components"
        else:
            coordinates = validation_z
            subtitle = "two latent dimensions"
        lower = np.percentile(coordinates, 1, axis=0)
        upper = np.percentile(coordinates, 99, axis=0)
        span = np.maximum(upper - lower, 1e-9)
        lower -= 0.08 * span
        upper += 0.08 * span
        x0, y0, x1, y1 = box
        draw.rectangle(box, outline="#303030", width=2)
        draw.text((x0, y0 - 27), f"{condition.latent_size}-D: {subtitle}", font=label_font, fill="#202020")
        for coordinate, label in zip(coordinates, validation_labels):
            px = x0 + (coordinate[0] - lower[0]) / (upper[0] - lower[0]) * (x1 - x0)
            py = y1 - (coordinate[1] - lower[1]) / (upper[1] - lower[1]) * (y1 - y0)
            px = min(max(px, x0 + 2), x1 - 2)
            py = min(max(py, y0 + 2), y1 - 2)
            draw.ellipse((px - 3, py - 3, px + 3, py + 3), fill=colors[int(label)])
    for digit, color in enumerate(colors):
        x = 350 + digit * 62
        draw.ellipse((x, 578, x + 12, 590), fill=color)
        draw.text((x + 17, 576), str(digit), font=small_font, fill="#303030")
    latent_plot.save(OUTPUT_ROOT / "latent_spaces.png")

    validation_labels = sweep.read_expected_labels(DATA_ROOT / "test" / "labels.csv")
    selected_indices = [validation_labels.index(digit) for digit in range(10)]
    reconstructions = {}
    for condition in CONDITIONS:
        _, reconstruction, _ = aligned_data(representative_run(results, condition), "test")
        reconstructions[condition.name] = reconstruction.reshape(-1, 32, 32)
    scale = 4
    tile = 32 * scale
    left = 95
    top = 68
    reconstruction_plot = Image.new("RGB", (left + 10 * tile, top + 3 * tile), "#f5f5f2")
    draw = ImageDraw.Draw(reconstruction_plot)
    draw.text((left, 12), "One held-out reconstruction per digit", font=title_font, fill="#202020")
    for digit in range(10):
        draw.text((left + digit * tile + tile // 2 - 5, 44), str(digit), font=label_font, fill="#202020")
    for row, label in enumerate(("Input", "10-D", "2-D")):
        draw.text((12, top + row * tile + tile // 2 - 8), label, font=label_font, fill="#202020")
    for column, index in enumerate(selected_indices):
        original = Image.open(DATA_ROOT / "test" / f"image_{index:05d}.pgm").convert("L")
        images = (
            original,
            Image.fromarray(np.uint8(np.clip(reconstructions["latent10"][index], 0, 1) * 255)),
            Image.fromarray(np.uint8(np.clip(reconstructions["latent2"][index], 0, 1) * 255)),
        )
        for row, image in enumerate(images):
            enlarged = image.resize((tile, tile), Image.Resampling.NEAREST).convert("RGB")
            reconstruction_plot.paste(enlarged, (left + column * tile, top + row * tile))
    reconstruction_plot.save(OUTPUT_ROOT / "reconstructions.png")


def main() -> None:
    args = parse_args()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    results = [
        run_condition(condition, replicate, args)
        for condition in CONDITIONS
        for replicate in range(1, args.replicates + 1)
    ]
    summaries = write_results(results)
    plot_results(results, summaries)
    print("\nSummary", flush=True)
    for summary in summaries:
        print(
            f"{summary['latent_size']}-D: "
            f"ridge={summary['validation_ridge_zscore_mean']:.1%} +/- "
            f"{summary['validation_ridge_zscore_stddev']:.1%}, "
            f"nearest={summary['validation_nearest_zscore_mean']:.1%} +/- "
            f"{summary['validation_nearest_zscore_stddev']:.1%}, "
            f"MAE={summary['validation_absolute_reconstruction_error_mean']:.4f} +/- "
            f"{summary['validation_absolute_reconstruction_error_stddev']:.4f}",
            flush=True,
        )


if __name__ == "__main__":
    main()
