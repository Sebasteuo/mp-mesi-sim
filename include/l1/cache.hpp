#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstdint>
#include <cstring>
#include <array>
#include <map>
#include <list>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <condition_variable>
#include "include/busmem/interfaces.hpp"
#include "include/l1/cacheMetrics.hpp"
#include "include/busmem/messagesTypes.hpp"

// Estados del protocolo MESI
enum class MESIState { MODIFIED, EXCLUSIVE, SHARED, INVALID };

// Representa una línea en la caché
struct CacheLine {
  MESIState state = MESIState::INVALID;
  uint64_t tag = 0;
  std::array<uint8_t, LINE_SIZE> data{};
};

class Cache final : public ICache {
public:
  Cache(int id);

  void setBus(IBus& bus_ref);

  // Lee un dato desde la caché. Bloquea si ocurre un fallo (miss).
  template <typename T> T read(uint64_t addr);

  // Escribe un dato en la caché. Bloquea si ocurre un fallo (miss).
  template <typename T> void write(uint64_t addr, const T& value);

  // Callback para cuando el bus envía datos a esta caché.
  SnoopResp snoop(const BusPacket& pkt) override;
  void onBusData(const BusPacket& pkt, bool is_shared) override;

  CacheMetrics getMetrics() const;

private:
  // Direccion
  uint64_t getTag(uint64_t addr) const;
  uint64_t getIndex(uint64_t addr) const;
  uint64_t getOffset(uint64_t addr) const;
  uint64_t getLineAddr(uint64_t addr) const;

  uint64_t reconstructAddr(uint64_t tag, uint64_t index) const;

  // Cache
  std::pair<CacheLine*, int> findLineInSet(uint64_t lineAddr);
  int findVictimWay(uint64_t index);
  void updateLru(uint64_t index, int way);

  // Métodos para manejar fallos de caché
  void handleReadMiss(uint64_t lineAddr);
  void handleWriteMiss(uint64_t lineAddr);

  void tickBusy(unsigned long cycles);

  int id;         // Identificador de esta caché
  IBus* bus;      // Referencia al bus para enviar mensajes
  std::mutex mtx; // Mutex para proteger el acceso concurrente a la caché
  std::condition_variable cv;

  CacheMetrics met;

  // Almacenamiento de la caché
  std::array<std::array<CacheLine, L1_CACHE_WAYS>, L1_CACHE_SETS> sets;
  std::array<std::list<int>, L1_CACHE_SETS> lru_ways;
};

// Implementación de las plantillas (templates)
template <typename T> T Cache::read(uint64_t addr) {
  if (sizeof(T) > LINE_SIZE) {
    throw std::runtime_error("El tipo de dato es más grande que el tamaño de la línea de caché.");
  }

  uint64_t lineAddr = getLineAddr(addr);
  size_t offset = getOffset(addr);
  uint64_t index = getIndex(lineAddr);

  std::unique_lock<std::mutex> lock(mtx);

  auto [line, way] = findLineInSet(lineAddr);
  bool did_miss = false;

  // MISS: La línea no está en la caché o es inválida
  while (line == nullptr || line->state == MESIState::INVALID) {
    if (!did_miss) {
      met.misses_r++;
      did_miss = true;
    }
    handleReadMiss(lineAddr);
    cv.wait(lock);
    std::tie(line, way) = findLineInSet(lineAddr);
  }

  // HIT: La línea está en la caché y es válida
  met.hits++;
  tickBusy(L1_HIT_LAT);
  updateLru(index, way);
  T value;
  memcpy(&value, &line->data[offset], sizeof(T));
  return value;
}

template <typename T> void Cache::write(uint64_t addr, const T& value) {
  if (sizeof(T) > LINE_SIZE) {
    throw std::runtime_error("El tipo de dato es más grande que el tamaño de la línea de caché.");
  }

  uint64_t lineAddr = getLineAddr(addr);
  size_t offset = getOffset(lineAddr);
  uint64_t index = getIndex(lineAddr);

  std::unique_lock<std::mutex> lock(mtx);
  bool did_miss = false;

  // MISS: La línea no está o es inválida
  while (true) {
    auto [line, way] = findLineInSet(lineAddr);

    if (line == nullptr || line->state == MESIState::INVALID) {
      // --- WRITE MISS (Desde I) ---
      // Write-Allocate
      if (!did_miss) {
        met.misses_w++;
        did_miss = true;
      }
      handleWriteMiss(lineAddr); // Envía BusUp
      cv.wait(lock);             // Espera a que onBusData() nos despierte

    } else if (line->state == MESIState::SHARED) {
      // --- UPGRADE MISS (Desde S) ---
      if (!did_miss) {
        met.misses_w++;
        did_miss = true;
      }
      handleWriteMiss(lineAddr); // Envía BusUp
      cv.wait(lock);             // Espera a que onBusData() nos despierte

    } else {
      // --- WRITE HIT (Desde E o M) ---
      // Write-Back
      met.hits++;
      tickBusy(L1_HIT_LAT);
      updateLru(index, way);
      line->state = MESIState::MODIFIED;
      memcpy(&line->data[offset], &value, sizeof(T));
      break;
    }
  }
}

#endif // CACHE_HPP