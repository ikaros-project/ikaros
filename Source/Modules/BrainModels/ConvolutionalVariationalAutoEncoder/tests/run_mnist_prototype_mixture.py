#!/usr/bin/env python3

"""Train a class-conditional prototype-mixture network on saved MNIST latent codes."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from typing import Any

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import run_mnist_direct_vae_full as full


OUTPUT_ROOT = full.FULL_OUTPUT_ROOT / "prototype_mixture"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ticks", type=int, default=600_000)
    parser.add_argument("--replicates", type=int, default=3)
    parser.add_argument("--seed-base", type=int, default=71_000)
    parser.add_argument("--prototypes-per-class", type=int, default=20)
    parser.add_argument("--epochs", type=int, default=30)
    parser.add_argument("--batch-size", type=int, default=512)
    parser.add_argument("--learning-rate", type=float, default=0.01)
    parser.add_argument("--width", type=float, default=2.0)
    parser.add_argument("--resume", action="store_true")
    return parser.parse_args()


def component_scores(
    values: np.ndarray,
    centers: np.ndarray,
    variance: float,
) -> np.ndarray:
    return -full.squared_distances(values, centers) / (2.0 * variance)


def logsumexp(values: np.ndarray, axis: int) -> np.ndarray:
    maximum = np.max(values, axis=axis, keepdims=True)
    return np.squeeze(maximum, axis=axis) + np.log(
        np.sum(np.exp(values - maximum), axis=axis)
    )


def predict(
    values: np.ndarray,
    centers: np.ndarray,
    prototypes_per_class: int,
    variance: float,
) -> np.ndarray:
    predictions: list[np.ndarray] = []
    for begin in range(0, len(values), 1024):
        scores = component_scores(values[begin:begin + 1024], centers, variance)
        class_scores = logsumexp(
            scores.reshape(len(scores), 10, prototypes_per_class), axis=2
        )
        predictions.append(np.argmax(class_scores, axis=1))
    return np.concatenate(predictions)


def initialize_centers(
    values: np.ndarray,
    labels: np.ndarray,
    prototypes_per_class: int,
    rng: np.random.Generator,
) -> np.ndarray:
    return np.concatenate(
        [
            values[
                rng.choice(
                    np.flatnonzero(labels == label),
                    size=prototypes_per_class,
                    replace=False,
                )
            ]
            for label in range(10)
        ]
    ).copy()


def loss_and_center_gradient(
    values: np.ndarray,
    labels: np.ndarray,
    centers: np.ndarray,
    prototypes_per_class: int,
    variance: float,
) -> tuple[float, np.ndarray]:
    scores = component_scores(values, centers, variance)
    class_component_scores = scores.reshape(len(values), 10, prototypes_per_class)
    all_log_normalizer = logsumexp(scores, axis=1)
    true_scores = class_component_scores[np.arange(len(values)), labels]
    true_log_normalizer = logsumexp(true_scores, axis=1)
    loss = float(np.mean(all_log_normalizer - true_log_normalizer))

    component_probabilities = np.exp(scores - all_log_normalizer[:, None])
    true_responsibilities = np.exp(true_scores - true_log_normalizer[:, None])
    component_gradient = component_probabilities
    component_gradient.reshape(len(values), 10, prototypes_per_class)[
        np.arange(len(values)), labels
    ] -= true_responsibilities
    center_gradient = (
        component_gradient.T @ values
        - np.sum(component_gradient, axis=0)[:, None] * centers
    ) / (variance * len(values))
    return loss, center_gradient


def train_classifier(
    train_values: np.ndarray,
    train_labels: np.ndarray,
    validation_values: np.ndarray,
    validation_labels: np.ndarray,
    args: argparse.Namespace,
    random_seed: int,
) -> tuple[np.ndarray, list[dict[str, float]]]:
    rng = np.random.default_rng(random_seed)
    centers = initialize_centers(
        train_values, train_labels, args.prototypes_per_class, rng
    )
    first_moment = np.zeros_like(centers)
    second_moment = np.zeros_like(centers)
    variance = args.width * args.width
    step = 0
    history: list[dict[str, float]] = []

    for epoch in range(1, args.epochs + 1):
        permutation = rng.permutation(len(train_values))
        losses = []
        for begin in range(0, len(permutation), args.batch_size):
            indices = permutation[begin:begin + args.batch_size]
            loss, gradient = loss_and_center_gradient(
                train_values[indices],
                train_labels[indices],
                centers,
                args.prototypes_per_class,
                variance,
            )
            losses.append(loss)
            step += 1
            first_moment = 0.9 * first_moment + 0.1 * gradient
            second_moment = 0.999 * second_moment + 0.001 * gradient * gradient
            corrected_first = first_moment / (1.0 - 0.9 ** step)
            corrected_second = second_moment / (1.0 - 0.999 ** step)
            centers -= args.learning_rate * corrected_first / (
                np.sqrt(corrected_second) + 1.0e-8
            )

        validation_predictions = predict(
            validation_values,
            centers,
            args.prototypes_per_class,
            variance,
        )
        validation_accuracy = full.sweep.evaluation.accuracy(
            validation_predictions, validation_labels
        )
        history.append(
            {
                "epoch": float(epoch),
                "training_loss": float(np.mean(losses)),
                "validation_accuracy": validation_accuracy,
            }
        )
        if epoch == 1 or epoch % 5 == 0 or epoch == args.epochs:
            print(
                f"  epoch {epoch:2d}: loss={np.mean(losses):.4f}, "
                f"validation={validation_accuracy:.2%}",
                flush=True,
            )
    return centers, history


def prototype_usage(
    values: np.ndarray,
    labels: np.ndarray,
    centers: np.ndarray,
    prototypes_per_class: int,
) -> tuple[int, int]:
    used = 0
    for label in range(10):
        class_centers = centers[
            label * prototypes_per_class:(label + 1) * prototypes_per_class
        ]
        assignments = np.argmin(
            full.squared_distances(values[labels == label], class_centers), axis=1
        )
        used += len(np.unique(assignments))
    return used, 10 * prototypes_per_class


def run_classifier(
    replicate: int,
    seed: int,
    args: argparse.Namespace,
) -> dict[str, Any]:
    run_id = f"full_dataset_constant_gate_005_r{replicate}_s{seed}_{args.ticks}"
    source_dir = full.FULL_OUTPUT_ROOT / run_id
    result_path = OUTPUT_ROOT / f"mixture_r{replicate}_s{seed}.json"
    centers_path = OUTPUT_ROOT / f"mixture_r{replicate}_s{seed}.npz"
    if args.resume and result_path.exists() and centers_path.exists():
        with result_path.open() as handle:
            return json.load(handle)

    print(f"Prototype mixture: CVAE seed {seed}", flush=True)
    train_values, train_labels, test_values, test_labels = full.load_normalized_codes(
        source_dir
    )
    centers, history = train_classifier(
        train_values,
        train_labels,
        test_values,
        test_labels,
        args,
        random_seed=seed + 300_000,
    )
    variance = args.width * args.width
    test_predictions = predict(
        test_values, centers, args.prototypes_per_class, variance
    )
    accuracy = full.sweep.evaluation.accuracy(test_predictions, test_labels)
    used, available = prototype_usage(
        train_values, train_labels, centers, args.prototypes_per_class
    )
    result: dict[str, Any] = {
        "replicate": replicate,
        "cvae_seed": seed,
        "classifier_seed": seed + 300_000,
        "prototypes_per_class": args.prototypes_per_class,
        "total_prototypes": available,
        "used_prototypes": used,
        "epochs": args.epochs,
        "batch_size": args.batch_size,
        "learning_rate": args.learning_rate,
        "width": args.width,
        "test_accuracy": accuracy,
        "final_training_loss": history[-1]["training_loss"],
        "history": history,
    }
    np.savez_compressed(
        centers_path,
        centers=centers,
        width=np.asarray([args.width]),
        prototypes_per_class=np.asarray([args.prototypes_per_class]),
    )
    with result_path.open("w") as handle:
        json.dump(result, handle, indent=2, sort_keys=True)
        handle.write("\n")
    print(f"  test={accuracy:.2%}, used prototypes={used}/{available}", flush=True)
    return result


def plot_summary(summary: dict[str, float]) -> Path:
    baseline_path = full.FULL_OUTPUT_ROOT / "full_dataset_constant_gate_summary.json"
    with baseline_path.open() as handle:
        baseline = json.load(handle)
    values = (
        (
            "20 k-means prototypes/digit",
            100.0 * baseline["validation_twenty_prototypes_per_class_zscore_mean"],
            "#4c9575",
        ),
        (
            "20 trainable mixture prototypes/digit",
            100.0 * summary["test_accuracy_mean"],
            "#8a6ca8",
        ),
        (
            "1-nearest neighbour",
            100.0 * baseline["validation_nearest_zscore_mean"],
            "#e9a03b",
        ),
    )
    width, height = 1120, 360
    image = Image.new("RGB", (width, height), "#f5f5f2")
    draw = ImageDraw.Draw(image)
    title_font = ImageFont.load_default(size=22)
    font = ImageFont.load_default(size=14)
    draw.text((30, 18), "Prototype-mixture classifier", font=title_font, fill="#202020")
    plot_left, plot_right = 370, 1040
    for tick in range(0, 101, 20):
        x = plot_left + tick / 100.0 * (plot_right - plot_left)
        draw.line((x, 70, x, height - 45), fill="#d5d5d0")
        draw.text((x - 8, 51), str(tick), font=font, fill="#404040")
    for index, (label, value, color) in enumerate(values):
        y = 100 + index * 70
        draw.text((25, y + 1), label, font=font, fill="#202020")
        x = plot_left + value / 100.0 * (plot_right - plot_left)
        draw.rectangle((plot_left, y, x, y + 18), fill=color)
        draw.text((x + 6, y + 1), f"{value:.1f}%", font=font, fill="#202020")
    draw.text((plot_left, height - 28), "test accuracy", font=font, fill="#404040")
    path = OUTPUT_ROOT / "prototype_mixture_comparison.png"
    image.save(path)
    return path


def main() -> None:
    args = parse_args()
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    results = [
        run_classifier(replicate, args.seed_base + replicate, args)
        for replicate in range(1, args.replicates + 1)
    ]
    with (OUTPUT_ROOT / "prototype_mixture_results.csv").open("w", newline="") as handle:
        columns = [name for name in results[0] if name != "history"]
        writer = csv.DictWriter(handle, fieldnames=columns)
        writer.writeheader()
        writer.writerows({name: result[name] for name in columns} for result in results)

    accuracies = np.asarray([result["test_accuracy"] for result in results])
    summary = {
        "runs": len(results),
        "prototypes_per_class": args.prototypes_per_class,
        "total_prototypes": 10 * args.prototypes_per_class,
        "test_accuracy_mean": float(np.mean(accuracies)),
        "test_accuracy_stddev": (
            float(np.std(accuracies, ddof=1)) if len(accuracies) > 1 else 0.0
        ),
        "used_prototypes_mean": float(
            np.mean([result["used_prototypes"] for result in results])
        ),
    }
    with (OUTPUT_ROOT / "prototype_mixture_summary.json").open("w") as handle:
        json.dump(summary, handle, indent=2, sort_keys=True)
        handle.write("\n")
    plot_path = plot_summary(summary)
    print(json.dumps(summary, indent=2, sort_keys=True), flush=True)
    print(f"Plot: {plot_path}", flush=True)


if __name__ == "__main__":
    main()
