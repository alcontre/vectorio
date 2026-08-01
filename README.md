# VectorMMIO

`VectorMMIO` is a C++11 static library for moving 32-bit words between memory
and PCIe memory-mapped I/O regions. It uses aligned scalar or SIMD-sized bursts
where supported to provide efficient MMIO reads and writes.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Test

```sh
ctest --test-dir build --output-on-failure
```