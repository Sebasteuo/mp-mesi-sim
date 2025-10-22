#include <cstdint>
#include <array>
#include <vector>
#include <iostream>
#include "interfaces.hpp"
#include "messagesTypes.hpp"
#include "memory_map.hpp"   // MemLayout


class SharedMemory final : public IMemory {
public:
    SharedMemory();

    explicit SharedMemory(const MemLayout& layout, bool guard_consts = true);

    void readLine(std::uint64_t lineAddr,
                  std::array<std::uint8_t, LINE_SIZE>& out) override;

    void writeLine(std::uint64_t lineAddr,
                   const std::array<std::uint8_t, LINE_SIZE>& in) override;

private:
    std::vector<std::uint8_t> mem;
    bool guard_consts_ = false;
    std::uint64_t consts_begin_ = 0;
    std::uint64_t consts_end_   = 0;

    inline void check_aligned(std::uint64_t addr) const;
    inline void check_bounds (std::uint64_t addr) const;
};
