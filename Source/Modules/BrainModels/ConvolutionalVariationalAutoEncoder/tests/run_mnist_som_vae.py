#!/usr/bin/env python3

"""Compare frozen latent SOMs and joint SOM-VAE-style fine-tuning on full MNIST.

This is an offline NumPy experiment, not an Ikaros module. It imports existing
direct dense VAE checkpoints and verifies their outputs before any training.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import time
from pathlib import Path

import numpy as np

import run_mnist_direct_vae_full as full


OUTPUT_ROOT = full.FULL_OUTPUT_ROOT / "som_vae"


def sigmoid(x: np.ndarray) -> np.ndarray:
    return np.exp(-np.logaddexp(0.0, -x))


def load_images(split: str, count: int) -> np.ndarray:
    archive = "train-images-idx3-ubyte.gz" if split == "train" else "t10k-images-idx3-ubyte.gz"
    images = full.read_idx_images(full.RAW_ROOT / archive, count)
    return np.stack([full.center_and_pad(image).ravel() for image in images]).astype(np.float32) / 255.0


def encode(x: np.ndarray, model: dict) -> np.ndarray:
    return x @ model["encoder"] + model["encoder_bias"]


def decode(z: np.ndarray, model: dict) -> np.ndarray:
    return sigmoid(z @ model["decoder"] + model["decoder_bias"])


def read_codes(source: Path, split: str, count: int) -> tuple[np.ndarray, np.ndarray, float]:
    prefix = "train" if split == "train" else "validation"
    rows, _ = full.sweep.evaluation.read_and_align_rows(
        source / f"{prefix}_codes.csv", full.FULL_DATA_ROOT / split / "labels.csv"
    )
    expected = full.sweep.evaluation.read_expected_labels(full.FULL_DATA_ROOT / split / "labels.csv")
    labels = full.sweep.evaluation.labels_from_rows(rows)
    if not np.array_equal(labels, expected):
        raise ValueError("Saved labels are not aligned over the complete split")
    codes = full.sweep.evaluation.column_matrix(rows[:count], "latent_mean")
    mae = np.mean([float(row["absolute_reconstruction_error"]) for row in rows[:count]])
    return codes, labels[:count], float(mae)


def import_model(source: Path, train: np.ndarray, test: np.ndarray) -> tuple[dict, dict, np.ndarray, np.ndarray]:
    state = json.loads((source / "model.state").read_text())["items"]

    def matrix(name: str) -> np.ndarray:
        matches = [value for key, value in state.items() if key.endswith("." + name)]
        if len(matches) != 1:
            raise ValueError(f"Expected one state matrix named {name}")
        return np.asarray(matches[0]["value"], dtype=np.float32).reshape(matches[0]["shape"])

    gates = np.clip(sigmoid(matrix("LATENT_GATE_LOGITS")) * 1.2 - 0.1, 0.0, 1.0)
    original = {
        "encoder": matrix("DIRECT_MEAN_WEIGHTS") * gates,
        "encoder_bias": matrix("DIRECT_MEAN_BIAS") * gates,
        "decoder": matrix("DIRECT_DECODER_WEIGHTS"),
        "decoder_bias": matrix("DIRECT_DECODER_BIAS"),
    }
    stored_train, train_labels, _ = read_codes(source, "train", len(train))
    stored_test, test_labels, stored_mae = read_codes(source, "test", len(test))
    codes = encode(train, original)
    test_codes = encode(test, original)
    max_error = float(max(np.max(np.abs(codes - stored_train)), np.max(np.abs(test_codes - stored_test))))
    original_mae = float(np.mean(np.abs(decode(test_codes, original) - test)))
    if max_error > 5e-5 or abs(original_mae - stored_mae) > 5e-6:
        raise ValueError(f"Ikaros parity failed: code error={max_error}, MAE={original_mae}, saved={stored_mae}")

    # Absorb frozen gates and train-only standardization into the linear layers.
    # Exactly closed dimensions cannot affect reconstruction and stay removed.
    mean = np.mean(codes, axis=0, dtype=np.float64).astype(np.float32)
    scale = np.std(codes, axis=0, dtype=np.float64).astype(np.float32)
    active = scale > 1e-12
    model = {
        "encoder": original["encoder"][:, active] / scale[active],
        "encoder_bias": (original["encoder_bias"][active] - mean[active]) / scale[active],
        "decoder": scale[active, None] * original["decoder"][active],
        "decoder_bias": mean @ original["decoder"] + original["decoder_bias"],
    }
    np.testing.assert_allclose(decode(encode(test[:128], model), model), decode(test_codes[:128], original), atol=2e-6)
    info = {"code_max_abs_error": max_error, "original_test_mae": original_mae,
            "saved_test_mae": stored_mae, "nonzero_latent_dimensions": int(np.sum(active)),
            "source_sha256": hashlib.sha256((source / "model.state").read_bytes()).hexdigest()}
    return model, info, train_labels, test_labels


def grid(side: int) -> tuple[np.ndarray, np.ndarray]:
    coordinates = np.indices((side, side)).reshape(2, -1).T
    distances = np.abs(coordinates[:, None] - coordinates[None, :]).sum(axis=2)
    return coordinates, (distances == 1).astype(np.float32)


def initialize_map(codes: np.ndarray, side: int, seed: int) -> np.ndarray:
    coordinates, _ = grid(side)
    values, vectors = np.linalg.eigh(np.cov(codes, rowvar=False))
    axes = vectors[:, -2:] * np.sqrt(np.maximum(values[-2:], 0.0))
    positions = (coordinates / (side - 1) * 4.0 - 2.0)
    noise = np.random.default_rng(seed).normal(0, 0.03, (side * side, codes.shape[1]))
    return (np.mean(codes, axis=0) + positions @ axes.T + noise).astype(np.float32)


def som_loss_gradient(z: np.ndarray, centers: np.ndarray, adjacency: np.ndarray,
                      commitment: float, topology: float) -> tuple[dict, np.ndarray, np.ndarray, np.ndarray]:
    distances = full.squared_distances(z, centers)
    winners = np.argmin(distances, axis=1)
    weights = topology * adjacency[winners]
    weights[np.arange(len(z)), winners] += commitment
    denominator = z.size
    center_gradient = 2.0 * (np.sum(weights, axis=0)[:, None] * centers - weights.T @ z) / denominator
    # Only commitment reaches the encoder; neighbourhood targets are detached.
    z_gradient = 2.0 * commitment * (z - centers[winners]) / denominator
    losses = {"commitment": float(commitment * np.sum(distances[np.arange(len(z)), winners]) / denominator),
              "topology": float(topology * np.sum(adjacency[winners] * distances) / denominator)}
    return losses, z_gradient, center_gradient, winners


def loss_gradient(x: np.ndarray, model: dict, centers: np.ndarray, adjacency: np.ndarray,
                  commitment: float, topology: float) -> tuple[dict, dict, np.ndarray]:
    z = encode(x, model)
    losses, dz, dc, winners = som_loss_gradient(z, centers, adjacency, commitment, topology)
    quantized = centers[winners]
    logits_e = z @ model["decoder"] + model["decoder_bias"]
    logits_q = quantized @ model["decoder"] + model["decoder_bias"]
    losses["continuous_bce"] = float(np.mean(np.logaddexp(0, logits_e) - x * logits_e))
    losses["quantized_bce"] = float(np.mean(np.logaddexp(0, logits_q) - x * logits_q))
    de = (sigmoid(logits_e) - x) / x.size
    dq = (sigmoid(logits_q) - x) / x.size
    dz += de @ model["decoder"].T
    np.add.at(dc, winners, dq @ model["decoder"].T)
    gradients = {"encoder": x.T @ dz, "encoder_bias": np.sum(dz, axis=0),
                 "decoder": z.T @ de + quantized.T @ dq,
                 "decoder_bias": np.sum(de + dq, axis=0)}
    return losses, gradients, dc


class Adam:
    def __init__(self, parameters: dict):
        self.m = {name: np.zeros_like(value) for name, value in parameters.items()}
        self.v = {name: np.zeros_like(value) for name, value in parameters.items()}
        self.step = 0

    def update(self, parameters: dict, gradients: dict, rates: dict) -> None:
        self.step += 1
        for name, value in parameters.items():
            grad = gradients[name]
            self.m[name] *= 0.9
            self.m[name] += 0.1 * grad
            self.v[name] *= 0.999
            self.v[name] += 0.001 * grad * grad
            value -= rates[name] * (self.m[name] / (1 - 0.9 ** self.step)) / (
                np.sqrt(self.v[name] / (1 - 0.999 ** self.step)) + 1e-8
            )


def train_model(x: np.ndarray, model: dict, centers: np.ndarray, side: int,
                args: argparse.Namespace, mode: str, seed: int, epochs: int) -> list[dict]:
    _, adjacency = grid(side)
    frozen = mode == "frozen_som"
    topology = 0.0 if mode == "no_topology" else args.topology
    parameters = {"centers": centers} if frozen else model | {"centers": centers}
    rates = {name: args.center_rate if name == "centers" else args.learning_rate for name in parameters}
    optimizer = Adam(parameters)
    rng = np.random.default_rng(seed)
    history = []
    codes = encode(x, model) if frozen else None
    for epoch in range(1, epochs + 1):
        start = time.monotonic()
        sums = {}
        indices = rng.permutation(len(x))
        for begin in range(0, len(x), args.batch_size):
            batch = indices[begin:begin + args.batch_size]
            if frozen:
                losses, _, dc, _ = som_loss_gradient(codes[batch], centers, adjacency, args.commitment, topology)
                gradients = {}
            else:
                losses, gradients, dc = loss_gradient(x[batch], model, centers, adjacency, args.commitment, topology)
            gradients["centers"] = dc
            optimizer.update(parameters, gradients, rates)
            for name, value in losses.items():
                sums[name] = sums.get(name, 0.0) + value * len(batch) / len(x)
        if not all(np.all(np.isfinite(value)) for value in parameters.values()):
            raise FloatingPointError("Non-finite trained parameter")
        history.append({"epoch": epoch, "seconds": time.monotonic() - start, **sums})
        print(f"  {mode} epoch {epoch}/{epochs}: loss={sum(sums.values()):.5f}, {history[-1]['seconds']:.1f}s", flush=True)
    return history


def assignments(z: np.ndarray, centers: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    winners, seconds, errors = [], [], []
    for begin in range(0, len(z), 1024):
        distance = full.squared_distances(z[begin:begin + 1024], centers)
        nearest = np.argsort(distance, axis=1, kind="stable")[:, :2]
        winners.extend(nearest[:, 0])
        seconds.extend(nearest[:, 1])
        errors.extend(np.sqrt(distance[np.arange(len(distance)), nearest[:, 0]]))
    return np.asarray(winners), np.asarray(seconds), np.asarray(errors)


def label_map(winners: np.ndarray, labels: np.ndarray, centers: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    counts = np.zeros((len(centers), 10), dtype=np.int64)
    np.add.at(counts, (winners, labels), 1)
    map_labels = np.argmax(counts, axis=1)
    occupied = np.flatnonzero(counts.sum(axis=1))
    empty = np.flatnonzero(counts.sum(axis=1) == 0)
    if len(empty):
        nearest = np.argmin(full.squared_distances(centers[empty], centers[occupied]), axis=1)
        map_labels[empty] = map_labels[occupied[nearest]]
    return map_labels, counts


def evaluate(model: dict, centers: np.ndarray, train: np.ndarray, test: np.ndarray,
             train_labels: np.ndarray, test_labels: np.ndarray, side: int) -> tuple[dict, dict]:
    train_z, test_z = encode(train, model), encode(test, model)
    train_winners, _, _ = assignments(train_z, centers)
    winners, seconds, errors = assignments(test_z, centers)
    map_labels, counts = label_map(train_winners, train_labels, centers)
    predictions = map_labels[winners]
    _, adjacency = grid(side)
    confusion = np.zeros((10, 10), dtype=int)
    np.add.at(confusion, (test_labels, predictions), 1)
    occupancy = counts.sum(axis=1)
    probabilities = occupancy[occupancy > 0] / len(train)
    continuous = decode(test_z, model)
    quantized = decode(centers[winners], model)
    result = {
        "test_accuracy": float(np.mean(predictions == test_labels)),
        "training_purity": float(np.sum(counts.max(axis=1)) / len(train)),
        "used_prototypes": int(np.count_nonzero(occupancy)),
        "effective_prototypes": float(np.exp(-np.sum(probabilities * np.log(probabilities)))),
        "test_topographic_error": float(np.mean(adjacency[winners, seconds] == 0)),
        "test_quantization_error": float(np.mean(errors)),
        "test_continuous_mae": float(np.mean(np.abs(continuous - test))),
        "test_quantized_mae": float(np.mean(np.abs(quantized - test))),
        "latent_stddev_mean": float(np.mean(np.std(train_z, axis=0))),
        "test_accuracy_per_digit": (np.diag(confusion) / confusion.sum(axis=1)).tolist(),
        "confusion_matrix": confusion.tolist(),
    }
    artifacts = {"map_labels": map_labels, "counts": counts, "occupancy": occupancy,
                 "decoded_centers": decode(centers, model), "test_predictions": predictions,
                 "test_winners": winners, "test_labels": test_labels,
                 "example_input": test[:24], "example_continuous": continuous[:24],
                 "example_quantized": quantized[:24]}
    return result, artifacts


def save_figures(directory: Path, side: int, artifacts: dict, result: dict) -> None:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.colors import BoundaryNorm

    fig, axes = plt.subplots(1, 3, figsize=(16, 6), constrained_layout=True)
    tile_size = 33
    mosaic = np.full((side * tile_size, side * tile_size), 0.35)
    for k, tile in enumerate(artifacts["decoded_centers"]):
        r, c = divmod(k, side)
        mosaic[r * tile_size:r * tile_size + 32, c * tile_size:c * tile_size + 32] = tile.reshape(32, 32)
    axes[0].imshow(mosaic, cmap="gray", vmin=0, vmax=1)
    axes[0].set_title("Decoded map prototypes")
    labels = artifacts["map_labels"].reshape(side, side)
    axes[1].imshow(labels, cmap="tab10", norm=BoundaryNorm(np.arange(-.5, 10.5), 10))
    for r in range(side):
        for c in range(side):
            axes[1].text(c, r, str(labels[r, c]), ha="center", va="center", color="white", fontsize=8)
    axes[1].set_title("Training-majority digit (post hoc)")
    heat = axes[2].imshow(artifacts["occupancy"].reshape(side, side), cmap="viridis")
    axes[2].set_title(f"Training occupancy: {result['used_prototypes']}/{side * side}")
    fig.colorbar(heat, ax=axes[2], shrink=.7)
    for ax in axes:
        ax.set_xticks([])
        ax.set_yticks([])
    fig.suptitle(f"{directory.name}, {side} x {side}: test accuracy {result['test_accuracy']:.2%}")
    fig.savefig(directory / "map.png", dpi=140)
    plt.close(fig)
    fig, axes = plt.subplots(3, 12, figsize=(14, 4), constrained_layout=True)
    for row, key in enumerate(("example_input", "example_continuous", "example_quantized")):
        for column in range(12):
            axes[row, column].imshow(artifacts[key][column].reshape(32, 32), cmap="gray", vmin=0, vmax=1)
            axes[row, column].set_xticks([])
            axes[row, column].set_yticks([])
        axes[row, 0].set_ylabel(("Input", "Continuous", "Prototype")[row])
    fig.savefig(directory / "reconstructions.png", dpi=150)
    plt.close(fig)


def summarize(results: list[dict], root: Path) -> None:
    metrics = ("test_accuracy", "test_continuous_mae", "test_quantized_mae", "used_prototypes",
               "effective_prototypes", "test_topographic_error", "latent_stddev_mean")
    summaries = []
    for side, mode in sorted({(row["side"], row["mode"]) for row in results}):
        rows = [row for row in results if row["side"] == side and row["mode"] == mode]
        item = {"side": side, "mode": mode, "runs": len(rows)}
        for metric in metrics:
            values = [row[metric] for row in rows]
            item[metric + "_mean"] = float(np.mean(values))
            item[metric + "_std"] = float(np.std(values, ddof=1)) if len(rows) > 1 else 0.0
        summaries.append(item)
    (root / "summary.json").write_text(json.dumps(summaries, indent=2) + "\n")
    with (root / "summary.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(summaries[0]))
        writer.writeheader()
        writer.writerows(summaries)
    import matplotlib.pyplot as plt
    fig, axes = plt.subplots(1, 3, figsize=(14, 4), constrained_layout=True)
    names = {"frozen_som": "Frozen SOM", "no_topology": "Joint, no topology", "joint_som": "Joint SOM"}
    colors = {"frozen_som": "#407b96", "no_topology": "#bf6c36", "joint_som": "#36865b"}
    for ax, metric, title, scale in zip(
        axes, ("test_accuracy", "test_topographic_error", "test_quantized_mae"),
        ("Test accuracy (%)", "Topographic error (%)", "Prototype reconstruction MAE"), (100, 100, 1)
    ):
        for mode in names:
            rows = [row for row in summaries if row["mode"] == mode]
            if not rows:
                continue
            ax.errorbar([row["side"] ** 2 for row in rows],
                        [scale * row[metric + "_mean"] for row in rows],
                        yerr=[scale * row[metric + "_std"] for row in rows],
                        marker="o", capsize=4, label=names[mode], color=colors[mode])
        ax.set_title(title)
        ax.set_xlabel("Prototype count")
        ax.grid(alpha=.2)
    axes[0].legend(fontsize=8)
    fig.savefig(root / "comparison.png", dpi=150)
    plt.close(fig)
    report = ["# SOM-VAE-style MNIST comparison", "",
              "Unsupervised map/encoder training; training labels assign map-cell labels only after training.",
              "Test labels are used only for final scoring. Values are means +/- sample standard deviations.", "",
              "| Prototypes | Condition | Runs | Test accuracy | Continuous MAE | Prototype MAE | Topographic error |",
              "| ---: | --- | ---: | ---: | ---: | ---: | ---: |"]
    for row in summaries:
        report.append(
            f"| {row['side'] ** 2} | {names[row['mode']]} | {row['runs']} | "
            f"{100 * row['test_accuracy_mean']:.2f} +/- {100 * row['test_accuracy_std']:.2f}% | "
            f"{row['test_continuous_mae_mean']:.4f} | {row['test_quantized_mae_mean']:.4f} | "
            f"{100 * row['test_topographic_error_mean']:.1f}% |"
        )
    report.extend(["", "Lower reconstruction and topographic errors are better.", "",
                   "![Comparison](comparison.png)", "", "## Maps and reconstructions", ""])
    for row in results:
        run = f"s{row['seed']}_{row['side']}x{row['side']}/{row['mode']}"
        report.append(f"- {run}: [map]({run}/map.png), [reconstructions]({run}/reconstructions.png), [metrics]({run}/result.json)")
    report.extend(["", "## Scope", "",
                   "This is an offline NumPy adaptation warm-started from Ikaros checkpoints, not a new Ikaros module.",
                   "The paper's squared reconstruction loss is replaced with Bernoulli cross-entropy.",
                   "Gates are frozen, and Gaussian sampling/prior regularization are not used during fine-tuning.",
                   "The no-topology control shares the SOM warm-up and disables neighbourhood loss only for fine-tuning.",
                   "The repeatedly inspected MNIST test set is not an untouched final benchmark.", "",
                   "Reference: [Fortuin et al., SOM-VAE, ICLR 2019](https://arxiv.org/abs/1806.02199).", ""])
    (root / "report.md").write_text("\n".join(report))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sides", type=int, nargs="+", default=[10, 16])
    parser.add_argument("--replicates", type=int, default=3)
    parser.add_argument("--epochs", type=int, default=20)
    parser.add_argument("--warmup-epochs", type=int, default=10)
    parser.add_argument("--batch-size", type=int, default=512)
    parser.add_argument("--learning-rate", type=float, default=1e-4)
    parser.add_argument("--center-rate", type=float, default=0.01)
    parser.add_argument("--commitment", type=float, default=0.1)
    parser.add_argument("--topology", type=float, default=0.01)
    parser.add_argument("--train-count", type=int, default=60_000)
    parser.add_argument("--test-count", type=int, default=10_000)
    parser.add_argument("--output", type=Path, default=OUTPUT_ROOT)
    args = parser.parse_args()
    if min(args.sides) < 2 or min(args.epochs, args.warmup_epochs, args.batch_size, args.replicates) < 1:
        parser.error("Grid sides must be >= 2 and counts must be positive")
    args.output.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(args.output / ".matplotlib"))
    os.environ.setdefault("XDG_CACHE_HOME", str(args.output / ".cache"))
    config = {key: str(value) if isinstance(value, Path) else value for key, value in vars(args).items()}
    (args.output / "config.json").write_text(json.dumps(config, indent=2) + "\n")
    print("Loading centered images", flush=True)
    train, test = load_images("train", args.train_count), load_images("test", args.test_count)
    results = []
    for replicate in range(1, args.replicates + 1):
        seed = 71_000 + replicate
        source = full.FULL_OUTPUT_ROOT / f"full_dataset_constant_gate_005_r{replicate}_s{seed}_600000"
        original, parity, train_labels, test_labels = import_model(source, train, test)
        print(f"Seed {seed}: Ikaros parity {parity}", flush=True)
        for side in args.sides:
            codes = encode(train, original)
            initial = initialize_map(codes, side, seed)
            print(f"Seed {seed}, {side} x {side}: shared frozen-map initialization", flush=True)
            warmup = train_model(train, original, initial, side, args, "frozen_som", seed + 100, args.warmup_epochs)
            for mode in ("frozen_som", "no_topology", "joint_som"):
                directory = args.output / f"s{seed}_{side}x{side}" / mode
                directory.mkdir(parents=True, exist_ok=True)
                model = {name: value.copy() for name, value in original.items()}
                centers = initial.copy()
                history = train_model(train, model, centers, side, args, mode, seed + 200, args.epochs)
                result, artifacts = evaluate(model, centers, train, test, train_labels, test_labels, side)
                result.update({"seed": seed, "side": side, "mode": mode, "parity": parity,
                               "warmup_history": warmup, "history": history})
                (directory / "result.json").write_text(json.dumps(result, indent=2) + "\n")
                np.savez_compressed(directory / "model.npz", **model, centers=centers, **artifacts)
                save_figures(directory, side, artifacts, result)
                print(f"RESULT {seed} {side} {mode}: {result['test_accuracy']:.2%}, topology error={result['test_topographic_error']:.3f}", flush=True)
                results.append(result)
                summarize(results, args.output)


if __name__ == "__main__":
    main()
