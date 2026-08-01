#ifndef VECTOR_MMIO_VECTOR_MMIO_HPP
#define VECTOR_MMIO_VECTOR_MMIO_HPP

#include <cstdint>

namespace VectorMMIO {

void write(volatile std::uint32_t *dst, const std::uint32_t *src,
           std::uint32_t words);

void read(std::uint32_t *dst, volatile std::uint32_t *src, std::uint32_t words);

} // namespace VectorMMIO

#endif // VECTOR_MMIO_VECTOR_MMIO_HPP