#include <VectorMMIO/VectorMMIO.hpp>

#include <cstdint>
#include <iostream>

#ifndef VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS
#define VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS 16
#endif

namespace {

const std::uint32_t kGuardWords = 16;
const std::uint32_t kMaxWords = 4 * VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS;
const std::uint32_t kDestinationWords = kGuardWords +
                                        VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS +
                                        kMaxWords + kGuardWords;
const std::uint32_t kDestinationSentinel = 0xdeadbeefU;

bool run_write_case(std::uint32_t destination_offset, std::uint32_t words) {
  alignas(64) std::uint32_t destination[kDestinationWords];
  std::uint32_t source[kMaxWords];

  for (std::uint32_t word = 0; word < kDestinationWords; ++word) {
    destination[word] = kDestinationSentinel;
  }
  for (std::uint32_t word = 0; word < kMaxWords; ++word) {
    source[word] = 0x10000000U + word;
  }

  volatile std::uint32_t *target =
      destination + kGuardWords + destination_offset;
  VectorMMIO::write(target, source, words);

  for (std::uint32_t word = 0; word < kDestinationWords; ++word) {
    const bool was_written = word >= kGuardWords + destination_offset &&
                             word < kGuardWords + destination_offset + words;
    const std::uint32_t expected =
        was_written ? source[word - kGuardWords - destination_offset]
                    : kDestinationSentinel;
    if (destination[word] != expected) {
      std::cerr << "max burst " << VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS
                << ", destination offset " << destination_offset << ", words "
                << words << ": mismatch at word " << word << std::endl;
      return false;
    }
  }

  return true;
}

bool run_read_case(std::uint32_t source_offset, std::uint32_t words) {
  alignas(64) std::uint32_t source[kDestinationWords];
  std::uint32_t destination[kMaxWords];

  for (std::uint32_t word = 0; word < kDestinationWords; ++word) {
    source[word] = kDestinationSentinel;
  }
  for (std::uint32_t word = 0; word < kMaxWords; ++word) {
    destination[word] = kDestinationSentinel;
    source[kGuardWords + source_offset + word] = 0x10000000U + word;
  }

  volatile std::uint32_t *target = source + kGuardWords + source_offset;
  VectorMMIO::read(destination, target, words);

  for (std::uint32_t word = 0; word < kMaxWords; ++word) {
    const std::uint32_t expected =
        word < words ? 0x10000000U + word : kDestinationSentinel;
    if (destination[word] != expected) {
      std::cerr << "read max burst " << VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS
                << ", source offset " << source_offset << ", words " << words
                << ": mismatch at word " << word << std::endl;
      return false;
    }
  }

  for (std::uint32_t word = 0; word < kDestinationWords; ++word) {
    const bool is_source = word >= kGuardWords + source_offset &&
                           word < kGuardWords + source_offset + kMaxWords;
    const std::uint32_t expected =
        is_source ? 0x10000000U + word - kGuardWords - source_offset
                  : kDestinationSentinel;
    if (source[word] != expected) {
      std::cerr << "read max burst " << VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS
                << ", source offset " << source_offset << ", words " << words
                << ": source changed at word " << word << std::endl;
      return false;
    }
  }

  return true;
}

} // namespace

int main() {
  for (std::uint32_t destination_offset = 0;
       destination_offset < VECTORMMIO_INSTR_SIM_MAX_BURST_WORDS;
       ++destination_offset) {
    for (std::uint32_t words = 0; words <= kMaxWords; ++words) {
      if (!run_write_case(destination_offset, words)) {
        return 1;
      }
      if (!run_read_case(destination_offset, words)) {
        return 1;
      }
    }
  }

  return 0;
}