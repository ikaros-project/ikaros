#!/usr/bin/env python3

"""Prepare paired augmented MNIST image sequences for CVAE consistency tests."""

from __future__ import annotations

import argparse
import random
from pathlib import Path

from prepare_mnist import (
    download_file,
    read_images,
    read_labels,
    repository_root,
    write_grayscale_png,
    MNIST_URLS,
)


SPLITS = {
    "train": ("train-images-idx3-ubyte.gz", "train-labels-idx1-ubyte.gz"),
    "test": ("t10k-images-idx3-ubyte.gz", "t10k-labels-idx1-ubyte.gz"),
}


def shifted_noisy_view(
    pixels: bytes,
    width: int,
    height: int,
    max_shift: int,
    noise: float,
    rng: random.Random,
) -> bytes:
    dx = rng.randint(-max_shift, max_shift)
    dy = rng.randint(-max_shift, max_shift)
    result = bytearray(width * height)
    for row in range(height):
        source_row = row - dy
        if source_row < 0 or source_row >= height:
            continue
        for col in range(width):
            source_col = col - dx
            if source_col < 0 or source_col >= width:
                continue
            value = pixels[source_row * width + source_col]
            if noise > 0.0:
                value = round(value + rng.gauss(0.0, noise * 255.0))
                value = min(255, max(0, value))
            result[row * width + col] = value
    return bytes(result)


def write_split(
    split: str,
    raw_dir: Path,
    output_dir: Path,
    limit: int,
    max_shift: int,
    noise: float,
    rng: random.Random,
) -> None:
    images_name, labels_name = SPLITS[split]
    rows, cols, images = read_images(raw_dir / images_name)
    labels = read_labels(raw_dir / labels_name)
    if len(images) != len(labels):
        raise RuntimeError(f"{split} images and labels have different counts")

    count = min(limit, len(images))
    split_dir = output_dir / split
    view_a_dir = split_dir / "view_a"
    view_b_dir = split_dir / "view_b"
    view_a_dir.mkdir(parents=True, exist_ok=True)
    view_b_dir.mkdir(parents=True, exist_ok=True)

    for index in range(count):
        write_grayscale_png(
            view_a_dir / f"image_{index:05d}.png",
            cols,
            rows,
            shifted_noisy_view(images[index], cols, rows, max_shift, noise, rng),
        )
        write_grayscale_png(
            view_b_dir / f"image_{index:05d}.png",
            cols,
            rows,
            shifted_noisy_view(images[index], cols, rows, max_shift, noise, rng),
        )

    with (split_dir / "labels.csv").open("w", newline="") as handle:
        handle.write("label\n")
        for label in labels[:count]:
            handle.write(f"{label}\n")

    print(f"Wrote {count} paired {split} images to {split_dir}")


def parse_args() -> argparse.Namespace:
    root = repository_root()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--raw-dir", type=Path, default=root / "UserData" / "cvae_mnist_full" / "raw")
    parser.add_argument("--output-dir", type=Path, default=root / "UserData" / "cvae_mnist_augmented_pairs")
    parser.add_argument("--train-count", type=int, default=60000)
    parser.add_argument("--test-count", type=int, default=10000)
    parser.add_argument("--max-shift", type=int, default=2)
    parser.add_argument("--noise", type=float, default=0.03)
    parser.add_argument("--seed", type=int, default=12345)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.train_count < 1 or args.test_count < 1:
        raise SystemExit("train-count and test-count must be positive")
    if args.max_shift < 0:
        raise SystemExit("max-shift must be non-negative")
    if args.noise < 0.0:
        raise SystemExit("noise must be non-negative")

    raw_dir = args.raw_dir.resolve()
    raw_dir.mkdir(parents=True, exist_ok=True)
    for name in MNIST_URLS:
        download_file(name, raw_dir / name)

    rng = random.Random(args.seed)
    output_dir = args.output_dir.resolve()
    write_split("train", raw_dir, output_dir, args.train_count, args.max_shift, args.noise, rng)
    write_split("test", raw_dir, output_dir, args.test_count, args.max_shift, args.noise, rng)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
