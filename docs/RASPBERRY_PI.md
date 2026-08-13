# Building Ikaros on Raspberry Pi

Ikaros builds and runs on 64-bit ARM Linux. These instructions target a
Raspberry Pi 4 or Raspberry Pi 5 running the 64-bit release of Raspberry Pi
OS Bookworm. The same package set and build were verified in an ARM64 Debian
Bookworm container.

Use the 64-bit operating system. You can confirm the architecture with:

```sh
uname -m
dpkg --print-architecture
```

The expected results are `aarch64` and `arm64`.

## Install the required packages

```sh
sudo apt update
sudo apt install \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libjpeg62-turbo-dev \
    liblapack-dev \
    libopenblas-dev \
    libssl-dev
```

Raspberry Pi OS and Debian use `libjpeg62-turbo-dev`. Ubuntu uses the
different package name `libjpeg-turbo8-dev`.

## Download and build Ikaros

```sh
git clone https://github.com/ikaros-project/ikaros.git
cd ikaros
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release
cmake --build Build --parallel 2
```

The executable is written to `Bin/ikaros`.

Using two parallel build processes is a conservative choice for Raspberry Pi
memory usage. A Raspberry Pi with more memory can use a larger value, while a
system under memory pressure should build with `--parallel 1`.

## Run Ikaros and the WebUI

Run a model by passing its `.ikg` file:

```sh
./Bin/ikaros path/to/model.ikg
```

To make the WebUI available to other computers on the local network:

```sh
./Bin/ikaros -B 0.0.0.0 -w 8000 path/to/model.ikg
```

Open `http://raspberry-pi-address:8000/` in a browser, replacing
`raspberry-pi-address` with the Pi's hostname or IP address. Binding to
`0.0.0.0` exposes the server on every network interface, so only do this on a
trusted network or protect access with the Ikaros authentication option.

For browser access only from the Pi itself, omit `-B 0.0.0.0`.

## Optional dependencies

The following packages enable the optional image, video, USB, camera, and
face-processing modules available on Linux:

```sh
sudo apt install \
    libavcodec-dev \
    libavdevice-dev \
    libavformat-dev \
    libavutil-dev \
    libdlib-dev \
    libpng-dev \
    libswresample-dev \
    libswscale-dev \
    libtiff-dev \
    libusb-1.0-0-dev \
    libwebp-dev
```

CMake detects these libraries automatically. Reconfigure and rebuild after
installing them:

```sh
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release
cmake --build Build --parallel 2
```

PNG, TIFF, and WebP support can be explicitly required:

```sh
cmake -S . -B Build \
    -DCMAKE_BUILD_TYPE=Release \
    -DIKAROS_PNG=ON \
    -DIKAROS_TIFF=ON \
    -DIKAROS_WEBP=ON
```

The Dynamixel modules require the Dynamixel SDK, which is not included in the
package commands above. Install a Linux ARM64 version of the SDK before
configuring Ikaros if those modules are needed.

`AudioInput`, `AudioOutput`, and `AppleVisionFaceDetector` depend on Apple
frameworks and are not built on Raspberry Pi.

## Run the tests

Install Python 3 and Node.js to run the complete test suite. Node.js is only
needed for the WebUI JavaScript tests.

```sh
sudo apt install python3 nodejs
cmake -S . -B Build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build Build-debug --parallel 2
python3 Source/Kernel/UnitTesting/KernelTests/kernel_test.py \
    --ikaros Bin/ikaros \
    --jobs 1
python3 Scripts/ci_webui_smoke.py --ikaros Bin/ikaros
```

The tests use a Debug build because some kernel regressions use Debug-only
failure injection. Running the tests serially reduces memory pressure and
avoids timing interference between WebUI tests on a slower Pi.

## Docker-based ARM64 testing

An ARM64 Debian container can check compilation and user-space behavior
before testing on physical hardware:

```sh
docker run --rm -it \
    --platform linux/arm64 \
    debian:bookworm
```

Docker testing does not reproduce Raspberry Pi hardware, kernel drivers, or
performance. A physical Pi is still required to validate GPIO, I2C, SPI,
cameras, USB devices, real-time timing, thermals, and sustained performance.

For general Linux information and compiler alternatives, see
[Building Ikaros on Linux](LINUX.md).
