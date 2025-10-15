#ifndef INTERFACES_HPP
#define INTERFACES_HPP

#include <cstdint>
#include <array>
#include "messages.hpp"

// Vista del bus para L1
struct IBus {
  virtual ~IBus() = default;
  virtual void enqueue(int l1_id, const BusPacket& req) = 0;
};

// Vista de la L1 para el Bus (para callbacks)
struct ICache {
  virtual ~ICache() = default;
  virtual void onBusData(const BusPacket& pkt, bool is_shared) = 0;
  virtual SnoopResp snoop(const BusPacket& pkt) = 0;
};

// Vista de memoria (líneas)
struct IMemory {
  virtual ~IMemory() = default;
  virtual void readLine(uint64_t lineAddr, std::array<uint8_t, LINE_SIZE>& out) = 0;
  virtual void writeLine(uint64_t lineAddr, const std::array<uint8_t, LINE_SIZE>& in) = 0;
};

#endif // INTERFACES_HPP