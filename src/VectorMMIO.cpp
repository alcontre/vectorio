#include <VectorMMIO/VectorMMIO.hpp>

#if defined(VECTORMMIO_INSTR_SIM)
#include <cassert>
#endif

#include <cstring>

// Determine the max burst size based on:
// Simulation mode (VECTORMMIO_INSTR_SIM) and desired simulation burst size,
// target architecture (x86_64, ARM, etc.),
// and compiler support for vector intrinsics (SSE2, AVX, AVX512, etc.).
// Instr set reqts for x86_64:
//    u32, u64 - Generic x86_64
//    u128     - sse2
//    u256     - avx
//    u512     - avx512

#if defined(VECTORMMIO_INSTR_SIM)
#if !defined(VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS)
#error "VECTORMMIO_INSTR_SIM requires VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS"
#elif VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS != 1 &&                             \
    VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS != 2 &&                               \
    VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS != 4 &&                               \
    VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS != 8 &&                               \
    VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS != 16
#error "VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS must be 1, 2, 4, 8, or 16"
#endif
#define MAX_BURST_WORDS VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS
#elif defined(VECTORMMIO_NO_BURST_WRITE)
#define MAX_BURST_WORDS 1
#else
#if defined(__AVX512F__) && defined(__AVX__) && defined(__SSE2__)
#define MAX_BURST_WORDS 16
#elif defined(__AVX__) && defined(__SSE2__)
#define MAX_BURST_WORDS 8
#elif defined(__SSE2__)
#define MAX_BURST_WORDS 4
#else
#define MAX_BURST_WORDS 2 // u32, u64
#endif
#endif

// Determine compiler intrinsics #includes for the target architecture
#if !defined(VECTORMMIO_INSTR_SIM)

#if (defined(__i386__) || defined(__x86_64__))
#if MAX_BURST_WORDS >= 8
#include <immintrin.h>
#elif MAX_BURST_WORDS >= 4
// SSE2 only
#include <emmintrin.h>
#endif
#endif

#endif // !defined(VECTORMMIO_INSTR_SIM)

////////////////////////////////////////////////////
// storeXX(), loadXX() helpers
////////////////////////////////////////////////////

#if defined(VECTORMMIO_INSTR_SIM)
static void sim_wr(volatile std::uint32_t *dst, const std::uint32_t *src,
                   std::uint32_t words) {
  assert((reinterpret_cast<uintptr_t>(dst) &
          (words * sizeof(std::uint32_t) - 1)) == 0);
  for (std::uint32_t word = 0; word < words; ++word) {
    dst[word] = src[word];
  }
}

static void sim_rd(std::uint32_t *dst, const volatile std::uint32_t *src,
                   std::uint32_t words) {
  assert((reinterpret_cast<uintptr_t>(src) &
          (words * sizeof(std::uint32_t) - 1)) == 0);
  for (std::uint32_t word = 0; word < words; ++word) {
    dst[word] = src[word];
  }
}
#endif // defined(VECTORMMIO_INSTR_SIM)

static void store32(volatile std::uint32_t *dst, const std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_wr(dst, src, 1);
#else
  *dst = *src;
#endif
}

static void load32(std::uint32_t *dst, const volatile std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_rd(dst, src, 1);
#else
  *dst = *src;
#endif
}

#if MAX_BURST_WORDS >= 2
static void store64(volatile std::uint32_t *dst, const std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_wr(dst, src, 2);
#else
  std::uint64_t value;
  std::memcpy(&value, src, sizeof(value));
  *reinterpret_cast<volatile std::uint64_t *>(dst) = value;
#endif
}

static void load64(std::uint32_t *dst, const volatile std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_rd(dst, src, 2);
#else
  const std::uint64_t value =
      *reinterpret_cast<const volatile std::uint64_t *>(src);
  std::memcpy(dst, &value, sizeof(value));
#endif
}
#endif // MAX_BURST_WORDS >= 2

#if MAX_BURST_WORDS >= 4
static void store128(volatile std::uint32_t *dst, const std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_wr(dst, src, 4);
#else
  auto *dst128v = reinterpret_cast<volatile __m128i *>(dst);
  auto *dst128 = const_cast<__m128i *>(dst128v);
  auto *src128 = reinterpret_cast<const __m128i *>(src);
  const __m128i reg = _mm_loadu_si128(src128);
  _mm_store_si128(dst128, reg);
#endif
}

static void load128(std::uint32_t *dst, const volatile std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_rd(dst, src, 4);
#else
  auto *src128v = reinterpret_cast<const volatile __m128i *>(src);
  auto *src128 = const_cast<const __m128i *>(src128v);
  const __m128i reg = _mm_load_si128(src128);
  _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), reg);
#endif
}
#endif // MAX_BURST_WORDS >= 4

#if MAX_BURST_WORDS >= 8
static void store256(volatile std::uint32_t *dst, const std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_wr(dst, src, 8);
#else
  auto *dst256v = reinterpret_cast<volatile __m256i *>(dst);
  auto *dst256 = const_cast<__m256i *>(dst256v);
  auto *src256 = reinterpret_cast<const __m256i *>(src);
  const __m256i reg = _mm256_loadu_si256(src256);
  _mm256_store_si256(dst256, reg);
#endif
}

static void load256(std::uint32_t *dst, const volatile std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_rd(dst, src, 8);
#else
  auto *src256v = reinterpret_cast<const volatile __m256i *>(src);
  auto *src256 = const_cast<const __m256i *>(src256v);
  const __m256i reg = _mm256_load_si256(src256);
  _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), reg);
#endif
}
#endif // MAX_BURST_WORDS >= 8

#if MAX_BURST_WORDS >= 16
static void store512(volatile std::uint32_t *dst, const std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_wr(dst, src, 16);
#else
  auto *dst512v = reinterpret_cast<volatile __m512i *>(dst);
  auto *dst512 = const_cast<__m512i *>(dst512v);
  auto *src512 = reinterpret_cast<const __m512i *>(src);
  const __m512i reg = _mm512_loadu_si512(src512);
  _mm512_store_si512(dst512, reg);
#endif
}

static void load512(std::uint32_t *dst, const volatile std::uint32_t *src) {
#if defined(VECTORMMIO_INSTR_SIM)
  sim_rd(dst, src, 16);
#else
  auto *src512v = reinterpret_cast<const volatile __m512i *>(src);
  auto *src512 = const_cast<const __m512i *>(src512v);
  const __m512i reg = _mm512_load_si512(src512);
  _mm512_storeu_si512(reinterpret_cast<__m512i *>(dst), reg);
#endif
}
#endif // MAX_BURST_WORDS >= 16

namespace VectorMMIO {

void write(volatile std::uint32_t *dst, const std::uint32_t *src,
           std::uint32_t words) {

#if MAX_BURST_WORDS >= 1
  if (words >= 1 && (reinterpret_cast<uintptr_t>(dst) & 0x4)) {
    store32(dst, src);
    dst++;
    src++;
    words--;
  }
#endif

#if MAX_BURST_WORDS >= 2
  if (words >= 2 && (reinterpret_cast<uintptr_t>(dst) & 0x8)) {
    store64(dst, src);
    dst += 2;
    src += 2;
    words -= 2;
  }
#endif
#if MAX_BURST_WORDS >= 4
  if (words >= 4 && (reinterpret_cast<uintptr_t>(dst) & 0x10)) {
    store128(dst, src);
    dst += 4;
    src += 4;
    words -= 4;
  }
#endif
#if MAX_BURST_WORDS >= 8
  if (words >= 8 && (reinterpret_cast<uintptr_t>(dst) & 0x20)) {
    store256(dst, src);
    dst += 8;
    src += 8;
    words -= 8;
  }
#endif
#if MAX_BURST_WORDS >= 16
  if (words >= 16 && (reinterpret_cast<uintptr_t>(dst) & 0x40)) {
    store512(dst, src);
    dst += 16;
    src += 16;
    words -= 16;
  }
#endif

  // Bulk transfer data at the maximum permissible burst size

  const auto num_max_burst_transfers = words / MAX_BURST_WORDS;
  for (std::uint32_t i = 0; i < num_max_burst_transfers; ++i) {
#if MAX_BURST_WORDS >= 16
    store512(dst, src);
    dst += 16;
    src += 16;
#elif MAX_BURST_WORDS >= 8
    store256(dst, src);
    dst += 8;
    src += 8;
#elif MAX_BURST_WORDS >= 4
    store128(dst, src);
    dst += 4;
    src += 4;
#elif MAX_BURST_WORDS >= 2
    store64(dst, src);
    dst += 2;
    src += 2;
#else
    store32(dst, src);
    dst++;
    src++;
#endif
  }
  words -= num_max_burst_transfers * MAX_BURST_WORDS;

  // Handle any remaining words that didn't fit into a full burst transfer

#if MAX_BURST_WORDS >= 16
  if (words >= 8) {
    store256(dst, src);
    dst += 8;
    src += 8;
    words -= 8;
  }
#endif
#if MAX_BURST_WORDS >= 8
  if (words >= 4) {
    store128(dst, src);
    dst += 4;
    src += 4;
    words -= 4;
  }
#endif
#if MAX_BURST_WORDS >= 4
  if (words >= 2) {
    store64(dst, src);
    dst += 2;
    src += 2;
    words -= 2;
  }
#endif
  if (words != 0) {
    store32(dst, src);
    dst++;
    src++;
    words--;
  }
}

void read(std::uint32_t *dst, const volatile std::uint32_t *src,
          std::uint32_t words) {

#if MAX_BURST_WORDS >= 1
  if (words >= 1 && (reinterpret_cast<uintptr_t>(src) & 0x4)) {
    load32(dst, src);
    dst++;
    src++;
    words--;
  }
#endif

#if MAX_BURST_WORDS >= 2
  if (words >= 2 && (reinterpret_cast<uintptr_t>(src) & 0x8)) {
    load64(dst, src);
    dst += 2;
    src += 2;
    words -= 2;
  }
#endif
#if MAX_BURST_WORDS >= 4
  if (words >= 4 && (reinterpret_cast<uintptr_t>(src) & 0x10)) {
    load128(dst, src);
    dst += 4;
    src += 4;
    words -= 4;
  }
#endif
#if MAX_BURST_WORDS >= 8
  if (words >= 8 && (reinterpret_cast<uintptr_t>(src) & 0x20)) {
    load256(dst, src);
    dst += 8;
    src += 8;
    words -= 8;
  }
#endif
#if MAX_BURST_WORDS >= 16
  if (words >= 16 && (reinterpret_cast<uintptr_t>(src) & 0x40)) {
    load512(dst, src);
    dst += 16;
    src += 16;
    words -= 16;
  }
#endif

  const auto num_max_burst_transfers = words / MAX_BURST_WORDS;
  for (std::uint32_t i = 0; i < num_max_burst_transfers; ++i) {
#if MAX_BURST_WORDS >= 16
    load512(dst, src);
    dst += 16;
    src += 16;
#elif MAX_BURST_WORDS >= 8
    load256(dst, src);
    dst += 8;
    src += 8;
#elif MAX_BURST_WORDS >= 4
    load128(dst, src);
    dst += 4;
    src += 4;
#elif MAX_BURST_WORDS >= 2
    load64(dst, src);
    dst += 2;
    src += 2;
#else
    load32(dst, src);
    dst++;
    src++;
#endif
  }
  words -= num_max_burst_transfers * MAX_BURST_WORDS;

#if MAX_BURST_WORDS >= 16
  if (words >= 8) {
    load256(dst, src);
    dst += 8;
    src += 8;
    words -= 8;
  }
#endif
#if MAX_BURST_WORDS >= 8
  if (words >= 4) {
    load128(dst, src);
    dst += 4;
    src += 4;
    words -= 4;
  }
#endif
#if MAX_BURST_WORDS >= 4
  if (words >= 2) {
    load64(dst, src);
    dst += 2;
    src += 2;
    words -= 2;
  }
#endif
  if (words != 0) {
    load32(dst, src);
  }
}

} // namespace VectorMMIO