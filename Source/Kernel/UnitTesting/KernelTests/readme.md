# Kernel Tests
 
This directory contains ikg files that test kernel functionality in Ikaros.

Run `python3 kernel_test.py` to check the kernel tests. Python-backed kernel tests
are in the sibling `PythonTests` directory and have their own runner.

Tests run in parallel, using 30 workers by default. Pass `--jobs N` (or
`-j N`) to choose another limit; use `--jobs 1` for sequential execution.

Files with a name starting with "test" are run by the script. Other files are supporting files that are not tested directly.

## ThreadSanitizer

Use a separate build and executable directory so the sanitizer executable does not replace the
normal `Bin/ikaros`:

```sh
cmake -S . -B Build-tsan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$PWD/Bin-tsan" \
    -DIKAROS_THREAD_SANITIZER=ON
cmake --build Build-tsan --parallel
TSAN_OPTIONS=halt_on_error=1 ./Bin-tsan/ikaros -b \
    Source/Kernel/UnitTesting/KernelTests/test_236_async_delay_history.ikg
TSAN_OPTIONS=halt_on_error=1 python3 \
    Source/Kernel/UnitTesting/KernelTests/kernel_test.py \
    --ikaros "$PWD/Bin-tsan/ikaros"
```

If Apple Clang's sanitizer runtime crashes before `main()`, configure with a current Homebrew LLVM
instead. Set both compilers during the first configuration of the build directory:

```sh
LLVM_ROOT="$(brew --prefix llvm)"
cmake -S . -B Build-tsan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$PWD/Bin-tsan" \
    -DCMAKE_CXX_COMPILER="$LLVM_ROOT/bin/clang++" \
    -DCMAKE_OBJCXX_COMPILER="$LLVM_ROOT/bin/clang++" \
    -DIKAROS_THREAD_SANITIZER=ON
```

## Delayed Propagation Release Benchmark

The delayed propagation benchmark is a timing probe, not a pass/fail test. It compares an identical
two-module control model with a model that propagates 64 scalar delay-2 connections per tick. Runs
are interleaved, and the script reports the paired median incremental time and nanoseconds per
connection.

Configure and build Ikaros in Release mode, then run the benchmark from the repository root:

```sh
cmake -S . -B Release -DCMAKE_BUILD_TYPE=Release
cmake --build Release --parallel
python3 Source/Kernel/UnitTesting/KernelTests/benchmark_delayed_propagation.py
```

The CMake project writes the executable to `Bin/ikaros`, so build the Release directory last. The
benchmark checks the selected CMake cache and refuses an executable that identifies itself as a
Debug build. Use `--help` to change the tick count, warm-up count, repetitions, executable, build
directory, or optional CSV output path.

The benchmark models start with `benchmark_`, so `kernel_test.py` does not include them in the
correctness suite.

## Image Conversion Release Benchmark

The image conversion benchmark compares the current row-at-a-time planar-float-to-RGB8 conversion
with a 32-row Accelerate implementation. It also compares the current scalar RGB8-to-planar-float
conversion with the corresponding block implementation. The default image size is 1920 by 1080.

Build Ikaros in Release mode and run:

```sh
cmake -S . -B Release -DCMAKE_BUILD_TYPE=Release
cmake --build Release --parallel
python3 Source/Kernel/UnitTesting/KernelTests/benchmark_image_conversion.py
```

The script reports median milliseconds per frame and speedups over seven measured processes. Use
`--width`, `--height`, `--iterations`, or `--block-rows` to explore other image and block sizes.
This benchmark requires Apple Accelerate support and is not included in the correctness suite.
