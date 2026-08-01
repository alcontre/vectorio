# VectorMMIO

`VectorMMIO` is a C++11 static library for moving 32-bit words between memory
and PCIe memory-mapped I/O regions. It uses aligned scalar or SIMD-sized bursts
where supported to provide efficient MMIO reads and writes.

## Transfer Width Control

Set the CMake option `VECTORMMIO_NO_BURST_WRITE` to use only 32-bit transfers
for both reads and writes, regardless of the target architecture or enabled
instruction-set extensions:

```sh
cmake -S . -B build -DVECTORMMIO_NO_BURST_WRITE=ON
```

Use this option when an MMIO aperture or attached peripheral does not support
wider accesses. The mapped region and peripheral must support the transfer width
selected by the build. Callers remain responsible for device-memory attributes
and any platform-specific memory barriers required by their device protocol.

## ARM Support

When the compiler enables ARM NEON/Advanced SIMD, `VectorMMIO` automatically
uses aligned 128-bit transfers. This includes the Cortex-A15 in TI Keystone II,
the Cortex-A72 in Versal Gen 1, and the Cortex-A78AE in Versal Gen 2. ARM builds
without NEON/Advanced SIMD use the 64-bit scalar transfer path, including
Versal R5/R52 real-time configurations where that extension is not enabled.

## x86 Support

`VectorMMIO` selects the widest available x86 transfer width from compiler
instruction-set macros. Enable the desired ISA with the compiler's target flags,
such as `-msse2`, `-mavx`, `-mavx512f`, or the corresponding `-march` setting.

| ISA support | Transfer width |
| --- | --- |
| No SSE2 | 64-bit scalar (2 words) |
| SSE2 | 128-bit (4 words) |
| AVX and SSE2 | 256-bit (8 words) |
| AVX512F, AVX, and SSE2 | 512-bit (16 words) |

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Test

```sh
ctest --test-dir build --output-on-failure
```