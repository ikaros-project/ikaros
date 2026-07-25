# Building Ikaros on Linux

Ikaros is continuously built and tested on Ubuntu 24.04 with both GCC and
Clang. GCC is the recommended compiler. Other current Linux distributions
should work when the equivalent development packages are installed, but the
package names and commands below are specific to Ubuntu and Debian.

## Install the required packages

```sh
sudo apt update
sudo apt install \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libjpeg-turbo8-dev \
    liblapack-dev \
    libopenblas-dev \
    libssl-dev
```

If another Debian-derived distribution does not provide
`libjpeg-turbo8-dev`, install its libjpeg-turbo development package.

## Download and build Ikaros

```sh
git clone https://github.com/ikaros-project/ikaros.git
cd ikaros
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release
cmake --build Build --parallel
```

The executable is written to `Bin/ikaros`.

To use Clang instead of GCC, install `clang` and select it when configuring a
new build directory:

```sh
CC=clang CXX=clang++ cmake -S . -B Build-clang -DCMAKE_BUILD_TYPE=Release
cmake --build Build-clang --parallel
```

## Run Ikaros and the WebUI

Run a model by passing its `.ikg` file:

```sh
./Bin/ikaros path/to/model.ikg
```

To start the WebUI explicitly on port 8000:

```sh
./Bin/ikaros -w 8000 path/to/model.ikg
```

Then open `http://localhost:8000/` in a browser. Use `-B 0.0.0.0` only when
the server must accept connections from outside the local machine or a
container.

## Optional dependencies

The following packages enable additional image, video, camera, and face
processing support:

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
cmake --build Build --parallel
```

PNG, TIFF, and WebP support can be explicitly required or disabled:

```sh
cmake -S . -B Build \
    -DIKAROS_PNG=ON \
    -DIKAROS_TIFF=ON \
    -DIKAROS_WEBP=ON
```

Each option accepts `AUTO`, `ON`, or `OFF`. `AUTO` is the default and enables
the codec when its development library is available.

The Dynamixel modules require the Dynamixel SDK, which is not installed by
the Ubuntu package commands above. Install that SDK separately before
configuring Ikaros if those modules are needed.

`AudioInput`, `AudioOutput`, and `AppleVisionFaceDetector` depend on Apple
frameworks and are not built on Linux.

## Run the tests

Install Python 3 and Node.js to run the complete test suite. Node.js is only
needed for the WebUI JavaScript tests.

```sh
sudo apt install python3 nodejs
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Debug
cmake --build Build --parallel
python3 Source/Kernel/UnitTesting/KernelTests/kernel_test.py \
    --ikaros Bin/ikaros \
    --jobs 2
python3 Scripts/ci_webui_smoke.py --ikaros Bin/ikaros
```

The Linux CI configuration in `.github/workflows/linux.yml` is the reference
for the currently tested Ubuntu version, compiler versions, and dependency
set.
