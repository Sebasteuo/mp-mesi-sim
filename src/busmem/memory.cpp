#include "include/busmem/memory.hpp"

SharedMemory::SharedMemory() : mem(MEM_BYTES, 0) {}

void SharedMemory::readLine(std::uint64_t lineAddr, std::array<std::uint8_t, LINE_SIZE>& out) {
  auto base = lineAddr;
  for (int i = 0; i < LINE_SIZE; i++)
    out[i] = mem[base + i];
}

void SharedMemory::writeLine(std::uint64_t lineAddr,
                             const std::array<std::uint8_t, LINE_SIZE>& in) {
  auto base = lineAddr;
  for (int i = 0; i < LINE_SIZE; i++)
    mem[base + i] = in[i];
}