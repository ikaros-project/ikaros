#!/usr/bin/env python3

import argparse
import math
import shutil
import subprocess
import tempfile
from pathlib import Path


def clamp_byte(value):
    return max(0, min(255, int(round(value * 255.0))))


def trajectory_parameters(tick, preset, period, vertical_amplitude, phase_offset):
    if preset == "single":
        return tick, period, vertical_amplitude, phase_offset

    if preset == "family":
        segment_length = 96
        periods = [80, 96, 112, 128]
        amplitudes = [0.16, 0.22, 0.26, 0.30]
        offsets = [0.0, 17.0, 31.0, 53.0]
        segment = (tick // segment_length) % len(periods)
        local_tick = tick % segment_length
        return local_tick, periods[segment], amplitudes[segment], offsets[segment]

    if preset == "gentle-heldout":
        return tick, 104, 0.24, 23.0

    if preset == "hard-heldout":
        return tick, 128, 0.32, 19.0

    raise ValueError(f"Unknown preset: {preset}")


def frame_pixels(width, height, tick, radius, sigma, preset, period, vertical_amplitude, phase_offset):
    motion_tick, period, vertical_amplitude, phase_offset = trajectory_parameters(
        tick,
        preset,
        period,
        vertical_amplitude,
        phase_offset,
    )
    phase_tick = motion_tick + phase_offset
    phase = phase_tick % period
    if phase < period / 2:
        cx = radius + phase / (period / 2 - 1) * (width - 2 * radius - 1)
    else:
        cx = width - radius - 1 - (phase - period / 2) / (period / 2 - 1) * (width - 2 * radius - 1)

    cy = height * 0.5 + math.sin(2.0 * math.pi * phase_tick / period) * height * vertical_amplitude
    pixels = bytearray()
    for y in range(height):
        for x in range(width):
            dx = x - cx
            dy = y - cy
            blob = math.exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma))
            halo = math.exp(-(dx * dx + dy * dy) / (2.0 * (sigma * 2.2) * (sigma * 2.2)))
            background = 0.02
            red = background + 0.93 * blob
            green = background + 0.78 * blob + 0.10 * halo
            blue = background + 0.52 * blob + 0.08 * halo
            pixels.extend((clamp_byte(red), clamp_byte(green), clamp_byte(blue)))
    return pixels


def write_ppm(path, width, height, pixels):
    with path.open("wb") as file:
        file.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
        file.write(pixels)


def generate_frames(directory, width, height, frames, radius, sigma, preset, period, vertical_amplitude, phase_offset):
    for tick in range(frames):
        write_ppm(
            directory / f"frame_{tick:04d}.ppm",
            width,
            height,
            frame_pixels(width, height, tick, radius, sigma, preset, period, vertical_amplitude, phase_offset),
        )


def encode_movie(ffmpeg, frame_directory, output, fps):
    output.parent.mkdir(parents=True, exist_ok=True)
    command = [
        ffmpeg,
        "-y",
        "-framerate",
        str(fps),
        "-i",
        str(frame_directory / "frame_%04d.ppm"),
        "-c:v",
        "libx264",
        "-crf",
        "0",
        "-preset",
        "veryfast",
        "-pix_fmt",
        "yuv420p",
        str(output),
    ]
    subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def main():
    parser = argparse.ArgumentParser(description="Generate a deterministic moving-blob movie for CVAE hierarchy tests.")
    parser.add_argument("--output", default="UserData/cvae_hierarchy_moving_blob/moving_blob.mp4")
    parser.add_argument("--width", type=int, default=64)
    parser.add_argument("--height", type=int, default=64)
    parser.add_argument("--frames", type=int, default=192)
    parser.add_argument("--fps", type=int, default=24)
    parser.add_argument("--radius", type=float, default=7.0)
    parser.add_argument("--sigma", type=float, default=3.5)
    parser.add_argument("--preset", choices=["single", "family", "gentle-heldout", "hard-heldout"], default="single")
    parser.add_argument("--period", type=int, default=96)
    parser.add_argument("--vertical-amplitude", type=float, default=0.22)
    parser.add_argument("--phase-offset", type=float, default=0.0)
    parser.add_argument("--ffmpeg", default=shutil.which("ffmpeg") or "/opt/homebrew/bin/ffmpeg")
    args = parser.parse_args()

    if args.width <= 0 or args.height <= 0 or args.frames <= 0:
        raise ValueError("width, height, and frames must be positive")
    if args.radius <= 0.0 or args.sigma <= 0.0:
        raise ValueError("radius and sigma must be positive")
    if args.period < 4:
        raise ValueError("period must be at least 4")
    if args.vertical_amplitude < 0.0:
        raise ValueError("vertical-amplitude must be non-negative")

    output = Path(args.output)
    with tempfile.TemporaryDirectory(prefix="cvae_moving_blob_") as temporary:
        frame_directory = Path(temporary)
        generate_frames(
            frame_directory,
            args.width,
            args.height,
            args.frames,
            args.radius,
            args.sigma,
            args.preset,
            args.period,
            args.vertical_amplitude,
            args.phase_offset,
        )
        encode_movie(args.ffmpeg, frame_directory, output, args.fps)

    print(output)


if __name__ == "__main__":
    main()
