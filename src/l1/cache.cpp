#include "include/l1/cache.hpp"
#include "include/l1/cacheMetrics.hpp"
#include <iostream>
#include <cstring>

constexpr int ilog2(int v) {
  return (v <= 1) ? 0 : 1 + ilog2(v >> 1);
}
constexpr int OFFSET_BITS = ilog2(LINE_SIZE);
constexpr int INDEX_BITS = ilog2(L1_CACHE_SETS);
constexpr uint64_t INDEX_MASK = (1ULL << INDEX_BITS) - 1;

Cache::Cache(int id) : id(id), bus(nullptr) {
  for (int i = 0; i < L1_CACHE_SETS; ++i) {
    for (int w = 0; w < L1_CACHE_WAYS; ++w) {
      lru_ways[i].push_back(w);
    }
  }
}

uint64_t Cache::getLineAddr(uint64_t addr) const {
  return addr & ~(LINE_SIZE - 1);
}
uint64_t Cache::getOffset(uint64_t addr) const {
  return addr & (LINE_SIZE - 1);
}
uint64_t Cache::getIndex(uint64_t addr) const {
  // Desplaza los bits de offset y aplica la máscara de índice
  return (addr >> OFFSET_BITS) & INDEX_MASK;
}
uint64_t Cache::getTag(uint64_t addr) const {
  // Desplaza los bits de offset e índice
  return addr >> (OFFSET_BITS + INDEX_BITS);
}
uint64_t Cache::reconstructAddr(uint64_t tag, uint64_t index) const {
  // Vuelve a ensamblar la dirección de la línea
  return (tag << (OFFSET_BITS + INDEX_BITS)) | (index << OFFSET_BITS);
}

std::pair<CacheLine*, int> Cache::findLineInSet(uint64_t lineAddr) {
  uint64_t tag = getTag(lineAddr);
  uint64_t index = getIndex(lineAddr);

  for (int way = 0; way < L1_CACHE_WAYS; ++way) {
    auto& line = sets[index][way];
    if (line.state != MESIState::INVALID && line.tag == tag) {
      return {&line, way}; // HIT
    }
  }
  return {nullptr, -1}; // MISS
}

int Cache::findVictimWay(uint64_t index) {
  for (int way = 0; way < L1_CACHE_WAYS; ++way) {
    if (sets[index][way].state == MESIState::INVALID) {
      return way;
    }
  }

  int lru_way = lru_ways[index].front();
  return lru_way;
}

void Cache::updateLru(uint64_t index, int way) {
  lru_ways[index].remove(way);
  lru_ways[index].push_back(way);
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
  uint64_t index = getIndex(lineAddr);
  uint64_t tag = getTag(lineAddr);

  // 1. Encontrar una vía para la nueva línea (Inválida o LRU)
  int way = findVictimWay(index);
  CacheLine& victim_line = sets[index][way];

  // 2. Si la línea víctima está sucia (MODIFIED), escribirla a memoria primero.
  if (victim_line.state == MESIState::MODIFIED) {
    uint64_t oldAddr = reconstructAddr(victim_line.tag, index);
    std::cout << "L1-" << id << ": Evicción con Write-Back de la dirección " << oldAddr
              << std::endl;
    BusPacket flush_pkt{BusMsgType::Flush, oldAddr, id, victim_line.data};
    bus->enqueue(id, flush_pkt);
  }

  // 3. Instalar la nueva línea
  victim_line.data = pkt.data;
  victim_line.tag = tag;
  victim_line.state = shared ? MESIState::SHARED : MESIState::EXCLUSIVE;

  std::cout << "L1-" << id << ": Recibidos datos para " << lineAddr
            << ". Nuevo estado: " << (shared ? "SHARED" : "EXCLUSIVE") << ". Instalado en set "
            << index << ", vía " << way << std::endl;

  // 4. Actualizar LRU
  updateLru(index, way);

  // 5. Despertar al hilo que espera
  cv.notify_all();
}

SnoopResp Cache::snoop(const BusPacket& pkt) {
  std::unique_lock<std::mutex> lock(mtx);
  uint64_t lineAddr = pkt.addrLine;
  uint64_t index = getIndex(lineAddr);

  auto [line, way] = findLineInSet(lineAddr);
  SnoopResp resp;

  if (line == nullptr) {
    resp.hasLine = false;
    return resp;
  }

  // Si hay hit, actualizar LRU
  updateLru(index, way);

  resp.hasLine = true;

  switch (pkt.type) {
  case BusMsgType::BusRd:
    std::cout << "L1-" << id << ": Snoop Hit en " << lineAddr << " para BusRd." << std::endl;
    if (line->state == MESIState::MODIFIED) {
      resp.isModified = true;
      resp.data = line->data;
      BusPacket flush_pkt{BusMsgType::Flush, lineAddr, id, line->data};
      bus->enqueue(id, flush_pkt);
    }
    line->state = MESIState::SHARED;
    resp.sharedHit = true;
    break;

  case BusMsgType::BusUp:
  case BusMsgType::Invalidate:
    met.inval_recv++;
    std::cout << "L1-" << id << ": Snoop Hit en " << lineAddr << " para "
              << (pkt.type == BusMsgType::BusUp ? "BusUp" : "Invalidate") << ". Invalidando línea."
              << std::endl;
    line->state = MESIState::INVALID;
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
}// Implementación simple: guardar el puntero al bus
void Cache::setBus(IBus& bus_ref) {
  bus = &bus_ref;
}
