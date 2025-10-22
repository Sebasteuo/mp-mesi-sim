#include <iostream>
#include "include/busmem/memory.hpp"

// ----- Constructores -----
SharedMemory::SharedMemory()
: mem(MEM_BYTES, 0) {}

SharedMemory::SharedMemory(const MemLayout& layout, bool guard_consts)
: mem(MEM_BYTES, 0),
  guard_consts_(guard_consts),
  consts_begin_(layout.BASE_CONSTS),
  consts_end_(layout.BASE_CONSTS + layout.SIZE_CONSTS) {}

// ----- Helpers -----
inline void SharedMemory::check_aligned(std::uint64_t addr) const {
    if (addr % LINE_SIZE != 0) {
        std::cerr << "[MEM][WARN] Dirección no alineada a LINE_SIZE: " << addr << "\n";
    }
}

inline void SharedMemory::check_bounds(std::uint64_t addr) const {
    if (addr + LINE_SIZE > MEM_BYTES) {
        throw std::out_of_range("[MEM] Acceso fuera de rango");
    }
}

// ----- IMemory -----
void SharedMemory::readLine(std::uint64_t lineAddr,
                            std::array<std::uint8_t, LINE_SIZE>& out) {
    check_aligned(lineAddr);
    check_bounds(lineAddr);

    std::cout << "[MEM] readLine addr=" << lineAddr << "\n";
    const auto base = lineAddr;
    for (int i = 0; i < LINE_SIZE; ++i) out[i] = mem[base + i];
}

void SharedMemory::writeLine(std::uint64_t lineAddr,
                             const std::array<std::uint8_t, LINE_SIZE>& in) {
    check_aligned(lineAddr);
    check_bounds(lineAddr);

    if (guard_consts_ && lineAddr >= consts_begin_ && (lineAddr + LINE_SIZE) <= consts_end_) {
        std::cout << "[MEM] writeLine into CONSTS blocked @ " << lineAddr << "\n";
        return;
    }

    std::cout << "[MEM] writeLine addr=" << lineAddr << "\n";
    const auto base = lineAddr;
    for (int i = 0; i < LINE_SIZE; ++i) mem[base + i] = in[i];
}
