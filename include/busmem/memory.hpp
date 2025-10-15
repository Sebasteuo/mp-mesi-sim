#include <cstdint>
#include <array>
#include <vector>
#include "interfaces.hpp"

class SharedMemory final : public IMemory {
public:
  SharedMemory();
  void readLine(std::uint64_t lineAddr, std::array<std::uint8_t, LINE_SIZE>& out) override;
  void writeLine(std::uint64_t lineAddr, const std::array<std::uint8_t, LINE_SIZE>& in) override;

private:
  std::vector<std::uint8_t> mem;
};
