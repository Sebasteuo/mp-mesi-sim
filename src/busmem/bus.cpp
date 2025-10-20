#include "include/busmem/bus.hpp"
#include <iostream>

Bus::Bus(IMemory& mem, std::array<ICache*, L1_COUNT> caches)
    : memory(mem), l1_caches(std::move(caches)) {}

void Bus::enqueue(int l1_id, const BusPacket& req) {
  std::lock_guard<std::mutex> lk(m);
  fifos[l1_id].push(req);
  cv.notify_one();
}

bool Bus::hasWork() const {
  for (int i = 0; i < L1_COUNT; i++)
    if (!fifos[i].empty())
      return true;
  return false;
}

void Bus::run() {
  for (;;) {
    BusPacket pkt{};
    int owner = -1;

    {
      std::unique_lock<std::mutex> lk(m);
      cv.wait(lk, [&] { return hasWork(); });
      for (int k = 0; k < L1_COUNT; k++) {
        int i = (rr_idx + k) % L1_COUNT;
        if (!fifos[i].empty()) {
          pkt = fifos[i].front();
          fifos[i].pop();
          owner = i;
          rr_idx = (i + 1) % L1_COUNT;
          break;
        }
      }
    }
    if (owner < 0)
      continue;
    process(owner, pkt);
  }
}

void Bus::process(int owner, const BusPacket& req) {
  tickBusy(BUS_LAT);

  std::vector<SnoopResp> resp(L1_COUNT);
  for (int i = 0; i < L1_COUNT; i++) {
    if (i == owner)
      continue;
    resp[i] = l1_caches[i]->snoop(req);
  }

  switch (req.type) {
  case BusMsgType::BusRd:
    handleBusRd(owner, req, resp);
    break;
  case BusMsgType::BusUp:
    handleBusUp(owner, req, resp);
    break;
  case BusMsgType::Invalidate:
    handleInv(owner, req, resp);
    break;
  case BusMsgType::Flush:
    handleFlush(owner, req, resp);
    break;
  case BusMsgType::Data:
    break; // no debería llegar desde L1
  }

  met.add(req);
}

void Bus::sendDataTo(int dst_l1, uint64_t lineAddr, const std::array<uint8_t, LINE_SIZE>& line,
                     bool shared) {
  BusPacket d{BusMsgType::Data, lineAddr, -1, line};
  met.add(d);
  // Llama al callback de la caché de destino
  if (l1_caches[dst_l1]) {
    l1_caches[dst_l1]->onBusData(d, shared);
  }
}

void Bus::broadcastInvalidateExcept(int owner, std::uint64_t lineAddr) {
  BusPacket inv{BusMsgType::Invalidate, lineAddr, owner, {}};
  // Las L1 reaccionan a la inval en su snoop/estado interno
  met.add(inv);
}

void Bus::handleBusRd(int owner, const BusPacket& req, const std::vector<SnoopResp>& resp) {
  bool is_shared = false;
  // ¿Alguna otra caché tiene la línea?
  for (int i = 0; i < L1_COUNT; i++) {
    if (i != owner && resp[i].hasLine) {
      is_shared = true;
      break;
    }
  }

  // Prioridad: si una caché tiene la línea en MODIFICADO, ella responde.
  for (int i = 0; i < L1_COUNT; i++)
    if (i != owner && resp[i].isModified) {
      tickBusy(BUS_LAT);
      sendDataTo(owner, req.addrLine, resp[i].data, true); // Es compartida
      return;
    }

  // Si no, la memoria responde.
  std::array<std::uint8_t, LINE_SIZE> line{};
  tickBusy(MEM_LAT);
  memory.readLine(req.addrLine, line);
  sendDataTo(owner, req.addrLine, line, is_shared); // Se informa si ya era compartida
}

void Bus::handleBusUp(int owner, const BusPacket& req, const std::vector<SnoopResp>& resp) {
  broadcastInvalidateExcept(owner, req.addrLine);
  for (int i = 0; i < L1_COUNT; i++)
    if (i != owner && resp[i].isModified) {
      tickBusy(BUS_LAT);
      // 'shared' es 'false' porque el 'owner' quiere la línea en EXCLUSIVE
      sendDataTo(owner, req.addrLine, resp[i].data, false);
      return;
    }

  // Si no, la memoria responde.
  std::array<std::uint8_t, LINE_SIZE> line{};
  tickBusy(MEM_LAT);
  memory.readLine(req.addrLine, line);
  // 'shared' es 'false' para que el 'owner' la reciba en EXCLUSIVE
  sendDataTo(owner, req.addrLine, line, false);
}

void Bus::handleInv(int owner, const BusPacket& req, const std::vector<SnoopResp>& /*resp*/) {
  // La difusión ya se contabiliza en broadcastInvalidateExcept
}

void Bus::handleFlush(int owner, const BusPacket& req, const std::vector<SnoopResp>& /*resp*/) {
  tickBusy(MEM_LAT);
  memory.writeLine(req.addrLine, req.data);
}

// Extras
void Bus::tickBusy(unsigned long cycles) {
  tick += cycles;
  std::cout << "Tiempo global del simulador: " << tick << " ciclos." << std::endl;
}
