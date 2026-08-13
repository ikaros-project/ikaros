# Building Ikaros on macOS

Ikaros builds and runs on modern macOS using Apple Clang and the system
Accelerate framework. These instructions target Apple Silicon Macs. Intel
Macs should use the same commands, with Homebrew installed for the Intel
architecture.

## Install the build tools

Install Apple's Command Line Tools:

```sh
xcode-select --install
```

The Command Line Tools provide Apple Clang, the macOS SDK, Git, Accelerate,
AudioToolbox, CoreMIDI, and the system libcurl development files used by
Ikaros.

Install [Homebrew](https://brew.sh/) if it is not already available, then
install CMake and the required JPEG library:

```sh
brew update
brew install cmake jpeg-turbo
```

Confirm that the tools are available:

```sh
clang++ --version
cmake --version
git --version
```

## Download and build Ikaros

```sh
git clone https://github.com/ikaros-project/ikaros.git
cd ikaros
cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release
cmake --build Build --parallel
```

The executable is written to `Bin/ikaros`.

Ikaros uses Apple Accelerate for optimized matrix operations by default. It
is normally unnecessary to install a separate BLAS or LAPACK implementation
on macOS.

## Run Ikaros and the WebUI

Run a model by passing its `.ikg` file:

```sh
./Bin/ikaros path/to/model.ikg
```

To start the WebUI explicitly on port 8000:

```sh
./Bin/ikaros -w 8000 path/to/model.ikg
```

Then open `http://localhost:8000/` in a browser.

To make the WebUI available to other computers on a trusted local network:

```sh
./Bin/ikaros -B 0.0.0.0 -w 8000 path/to/model.ikg
```

macOS may ask whether Ikaros should accept incoming connections. Binding to
`0.0.0.0` exposes the server on every network interface, so use local binding
or enable Ikaros authentication when remote access is not required or trusted.

## Optional dependencies

The following Homebrew packages enable additional image, video, USB, camera,
and face-processing modules:

```sh
brew install \
    dlib \
    ffmpeg \
    libpng \
    libtiff \
    libusb \
    webp
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
    -DCMAKE_BUILD_TYPE=Release \
    -DIKAROS_PNG=ON \
    -DIKAROS_TIFF=ON \
    -DIKAROS_WEBP=ON
```

Each codec option accepts `AUTO`, `ON`, or `OFF`. `AUTO` is the default and
enables the codec when its library is available.

The Dynamixel modules require the Dynamixel SDK. Install its macOS library and
headers separately before configuring Ikaros if those modules are needed.

The `AudioInput`, `AudioOutput`, and `AppleVisionFaceDetector` modules use
Apple frameworks and are available only in macOS builds.

## Run the tests

Install Python and Node.js to run the complete test suite. Node.js is only
needed for the WebUI JavaScript tests.

```sh
brew install python node
cmake -S . -B Build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build Build-debug --parallel
python3 Source/Kernel/UnitTesting/KernelTests/kernel_test.py \
    --ikaros Bin/ikaros \
    --jobs 2
python3 Scripts/ci_webui_smoke.py --ikaros Bin/ikaros
```

The tests use a Debug build because some kernel regressions use Debug-only
failure injection. Both build directories write the executable to
`Bin/ikaros`, so the most recently built configuration is the one that runs.

### Run ThreadSanitizer with Homebrew LLVM

Apple Clang's ThreadSanitizer runtime can fail before `main()` on some macOS
versions. Ikaros therefore provides a separate preset that uses Homebrew LLVM.
Install LLVM, configure the preset, and build it:

```sh
brew install llvm
cmake --preset macos-homebrew-tsan
cmake --build --preset macos-homebrew-tsan --parallel
```

The preset asks Homebrew for LLVM's installation prefix, so the same command
works with the normal Apple Silicon and Intel Homebrew locations. It writes
the instrumented executable to `Bin-tsan-homebrew/ikaros` without replacing
the normal `Bin/ikaros` executable.

Run a focused model or the complete kernel suite with the instrumented binary:

```sh
./Bin-tsan-homebrew/ikaros -b path/to/model.ikg
python3 Source/Kernel/UnitTesting/KernelTests/kernel_test.py \
    --ikaros Bin-tsan-homebrew/ikaros \
    --jobs 2
```

ThreadSanitizer substantially increases execution time and memory use. Start
with the smallest relevant test before running the complete suite.

For general Unix build information and CMake codec controls, see
[Building Ikaros on Linux](LINUX.md).
