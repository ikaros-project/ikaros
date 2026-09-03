#!/usr/bin/env python3

"""Prepare MNIST image sequences for Ikaros CVAE examples."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import struct
import sys
import urllib.request
import zlib
from pathlib import Path


MNIST_URLS = {
    "train-images-idx3-ubyte.gz": [
        "https://yann.lecun.com/exdb/mnist/train-images-idx3-ubyte.gz",
        "https://storage.googleapis.com/cvdf-datasets/mnist/train-images-idx3-ubyte.gz",
    ],
    "train-labels-idx1-ubyte.gz": [
        "https://yann.lecun.com/exdb/mnist/train-labels-idx1-ubyte.gz",
        "https://storage.googleapis.com/cvdf-datasets/mnist/train-labels-idx1-ubyte.gz",
    ],
    "t10k-images-idx3-ubyte.gz": [
        "https://yann.lecun.com/exdb/mnist/t10k-images-idx3-ubyte.gz",
        "https://storage.googleapis.com/cvdf-datasets/mnist/t10k-images-idx3-ubyte.gz",
    ],
    "t10k-labels-idx1-ubyte.gz": [
        "https://yann.lecun.com/exdb/mnist/t10k-labels-idx1-ubyte.gz",
        "https://storage.googleapis.com/cvdf-datasets/mnist/t10k-labels-idx1-ubyte.gz",
    ],
}

MNIST_SHA256 = {
    "train-images-idx3-ubyte.gz": "440fcabf73cc546fa21475e81ea370265605f56be210a4024d2ca8f203523609",
    "train-labels-idx1-ubyte.gz": "3552534a0a558bbed6aed32b30c495cca23d567ec52cac8be1a0730e8010255c",
    "t10k-images-idx3-ubyte.gz": "8d422c7b0a1c1c79245a5bcf07fe86e33eeafee792b84584aec276f5a2dbc4e6",
    "t10k-labels-idx1-ubyte.gz": "f7ae60f92e00ec6debd23a6088c31dbd2371eca3ffa0defaefb259924204aec6",
}


def repository_root() -> Path:
    return Path(__file__).resolve().parents[4]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def download_file(name: str, target: Path) -> None:
    if target.exists():
        actual = sha256(target)
        if actual == MNIST_SHA256[name]:
            return
        raise RuntimeError(
            f"{target} exists but has SHA-256 {actual}, expected {MNIST_SHA256[name]}"
        )

    errors: list[str] = []
    data = b""
    for url in MNIST_URLS[name]:
        print(f"Downloading {name} from {url}", file=sys.stderr)
        try:
            with urllib.request.urlopen(url, timeout=60) as response:
                data = response.read()
            break
        except OSError as error:
            errors.append(f"{url}: {error}")
    else:
        raise RuntimeError("Could not download " + name + "\n" + "\n".join(errors))
    actual = hashlib.sha256(data).hexdigest()
    if actual != MNIST_SHA256[name]:
        raise RuntimeError(f"{name} has SHA-256 {actual}, expected {MNIST_SHA256[name]}")
    target.write_bytes(data)


def read_images(path: Path) -> tuple[int, int, list[bytes]]:
    with gzip.open(path, "rb") as handle:
        magic, count, rows, cols = struct.unpack(">IIII", handle.read(16))
        if magic != 2051:
            raise RuntimeError(f"{path} is not an IDX image file")
        image_size = rows * cols
        images = [handle.read(image_size) for _ in range(count)]
    if any(len(image) != image_size for image in images):
        raise RuntimeError(f"{path} ended before all images were read")
    return rows, cols, images


def read_labels(path: Path) -> list[int]:
    with gzip.open(path, "rb") as handle:
        magic, count = struct.unpack(">II", handle.read(8))
        if magic != 2049:
            raise RuntimeError(f"{path} is not an IDX label file")
        labels = list(handle.read(count))
    if len(labels) != count:
        raise RuntimeError(f"{path} ended before all labels were read")
    return labels


def png_chunk(name: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + name
        + data
        + struct.pack(">I", zlib.crc32(name + data) & 0xFFFFFFFF)
    )


def write_grayscale_png(path: Path, width: int, height: int, pixels: bytes) -> None:
    rows = bytearray()
    for row in range(height):
        rows.append(0)
        start = row * width
        rows.extend(pixels[start:start + width])
    header = struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)
    data = (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", header)
        + png_chunk(b"IDAT", zlib.compress(bytes(rows), level=6))
        + png_chunk(b"IEND", b"")
    )
    path.write_bytes(data)


def write_split(
    split: str,
    images_name: str,
    labels_name: str,
    raw_dir: Path,
    output_dir: Path,
    limit: int,
) -> None:
    rows, cols, images = read_images(raw_dir / images_name)
    labels = read_labels(raw_dir / labels_name)
    if len(images) != len(labels):
        raise RuntimeError(f"{split} images and labels have different counts")

    count = min(limit, len(images))
    split_dir = output_dir / split
    split_dir.mkdir(parents=True, exist_ok=True)
    for index in range(count):
        write_grayscale_png(split_dir / f"image_{index:05d}.png", cols, rows, images[index])

    with (split_dir / "labels.csv").open("w", newline="") as handle:
        handle.write("label\n")
        for label in labels[:count]:
            handle.write(f"{label}\n")

    with (split_dir / "manifest.csv").open("w", newline="") as handle:
        handle.write("index,filename,label\n")
        for index, label in enumerate(labels[:count]):
            handle.write(f"{index},image_{index:05d}.png,{label}\n")

    print(f"Wrote {count} {split} images to {split_dir}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    default_output = repository_root() / "UserData" / "cvae_mnist"
    parser.add_argument("--output-dir", type=Path, default=default_output)
    parser.add_argument("--train-count", type=int, default=1000)
    parser.add_argument("--test-count", type=int, default=200)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.train_count < 1 or args.test_count < 1:
        raise SystemExit("train-count and test-count must be positive")

    output_dir = args.output_dir.resolve()
    raw_dir = output_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)

    for name in MNIST_URLS:
        download_file(name, raw_dir / name)

    write_split(
        "train",
        "train-images-idx3-ubyte.gz",
        "train-labels-idx1-ubyte.gz",
        raw_dir,
        output_dir,
        args.train_count,
    )
    write_split(
        "test",
        "t10k-images-idx3-ubyte.gz",
        "t10k-labels-idx1-ubyte.gz",
        raw_dir,
        output_dir,
        args.test_count,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
