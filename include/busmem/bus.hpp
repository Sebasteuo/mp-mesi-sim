#ifndef BUS_HPP
#define BUS_HPP

#include <queue>
#include <array>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "busMetrics.hpp"
#include "interfaces.hpp"

class Bus : public IBus {
public:
  Bus(IMemory& mem, std::array<ICache*, L1_COUNT> caches);
  void enqueue(int l1_id, const BusPacket& req);
  void run(); // loop del bus
  void stop();

  void tickBusy(unsigned long cycles);

private:
  bool hasWork() const;
  void process(int owner, const BusPacket& req);

  // helpers
  void sendDataTo(int dst_l1, uint64_t lineAddr, const std::array<uint8_t, LINE_SIZE>& line,
                  bool shared);
  void broadcastInvalidateExcept(int owner, std::uint64_t lineAddr);

  // handlers
  void handleBusRd(int owner, const BusPacket& req, const std::vector<SnoopResp>& resp);
  void handleBusUp(int owner, const BusPacket& req, const std::vector<SnoopResp>& resp);
  void handleInv(int owner, const BusPacket& req, const std::vector<SnoopResp>& resp);
  void handleFlush(int owner, const BusPacket& req, const std::vector<SnoopResp>& resp);

private:
  IMemory& memory;
  std::array<std::queue<BusPacket>, L1_COUNT> fifos;
  std::array<ICache*, L1_COUNT> l1_caches;

  std::mutex m;
  std::condition_variable cv;
  int rr_idx = 0;
  std::uint64_t sim_time = 0;
  BusMetrics met;
  std::atomic<bool> stop_requested_{false};

  // Extra

  unsigned long tick = 0;
};

#endif // BUS_HPP
