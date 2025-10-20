#include "include/l1/cache.hpp"
#include "include/l1/cacheMetrics.hpp"
#include <iostream>
#include <cstring>

Cache::Cache(int id) : id(id), bus(nullptr) {}

void Cache::setBus(IBus& bus_ref) {
  this->bus = &bus_ref;
}

uint64_t Cache::getLineAddr(uint64_t addr) const {
  return addr & ~(LINE_SIZE - 1);
}

size_t Cache::getOffset(uint64_t addr) const {
  return addr & (LINE_SIZE - 1);
}

void Cache::handleReadMiss(uint64_t lineAddr) {
  std::cout << "L1-" << id << ": Read Miss en dirección " << lineAddr << ". Enviando BusRd."
            << std::endl;
  BusPacket pkt{BusMsgType::BusRd, lineAddr, id};
  bus->enqueue(id, pkt);
}

void Cache::handleWriteMiss(uint64_t lineAddr) {
  std::cout << "L1-" << id << ": Write Miss en dirección " << lineAddr << ". Enviando BusUp (RFO)."
            << std::endl;
  met.inval_sent++;
  // Read-For-Ownership
  BusPacket pkt{BusMsgType::BusUp, lineAddr, id};
  bus->enqueue(id, pkt);
}

void Cache::onBusData(const BusPacket& pkt, bool shared) {
  std::unique_lock<std::mutex> lock(mtx);

  uint64_t lineAddr = pkt.addrLine;
  auto& line = lines[lineAddr]; // Crea o accede a la línea

  line.data = pkt.data;

  // Determinar el estado basado en si la línea fue compartida por otra caché
  // La lógica del bus debe determinar esto durante el snooping.
  if (shared) {
    line.state = MESIState::SHARED;
  } else {
    line.state = MESIState::EXCLUSIVE;
  }

  std::cout << "L1-" << id << ": Recibidos datos para " << lineAddr
            << ". Nuevo estado: " << (shared ? "SHARED" : "EXCLUSIVE") << std::endl;

  cv.notify_all();
}

SnoopResp Cache::snoop(const BusPacket& pkt) {
  std::unique_lock<std::mutex> lock(mtx);
  uint64_t lineAddr = pkt.addrLine;
  auto it = lines.find(lineAddr);

  SnoopResp resp;

  if (it == lines.end() || it->second.state == MESIState::INVALID) {
    resp.hasLine = false;
    return resp;
  }

  resp.hasLine = true;
  CacheLine& line = it->second;

  switch (pkt.type) {
  case BusMsgType::BusRd:
    // Otra caché está leyendo.
    std::cout << "L1-" << id << ": Snoop Hit en " << lineAddr << " para BusRd." << std::endl;
    if (line.state == MESIState::MODIFIED) {
      resp.isModified = true;
      resp.data = line.data;

      BusPacket flush_pkt{BusMsgType::Flush, lineAddr, id, line.data};
      bus->enqueue(id, flush_pkt);
    }
    // Tanto en Modified como en Exclusive, pasamos a Shared.
    line.state = MESIState::SHARED;
    resp.sharedHit = true;
    break;

  case BusMsgType::BusUp:
  case BusMsgType::Invalidate:
    // Otra caché quiere escribir. Debemos invalidar nuestra copia.
    met.inval_recv++;
    std::cout << "L1-" << id << ": Snoop Hit en " << lineAddr << " para "
              << (pkt.type == BusMsgType::BusUp ? "BusUp" : "Invalidate") << ". Invalidando línea."
              << std::endl;
    line.state = MESIState::INVALID;
    cv.notify_all();
    break;

  default:
    break;
  }

  return resp;
}

void Cache::tickBusy(unsigned long cycles) {
  met.busy_ticks += cycles;
}

// CacheMetrics m = caches[i]->getMetrics();
CacheMetrics Cache::getMetrics() const {
  std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
  return met;
}