#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstdint>
#include <cstring>
#include <array>
#include <map>
#include <mutex>
#include <stdexcept>
#include "include/busmem/interfaces.hpp"

// Estados del protocolo MESI
enum class MESIState { MODIFIED, EXCLUSIVE, SHARED, INVALID };

// Representa una línea en la caché
struct CacheLine {
  MESIState state = MESIState::INVALID;
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

private:
  uint64_t getLineAddr(uint64_t addr) const;
  size_t getOffset(uint64_t addr) const;

  // Métodos para manejar fallos de caché
  void handleReadMiss(uint64_t lineAddr);
  void handleWriteMiss(uint64_t lineAddr);

  int id;         // Identificador de esta caché
  IBus* bus;      // Referencia al bus para enviar mensajes
  std::mutex mtx; // Mutex para proteger el acceso concurrente a la caché

  // Almacenamiento de la caché
  std::map<uint64_t, CacheLine> lines;
};

// Implementación de las plantillas (templates)
template <typename T> T Cache::read(uint64_t addr) {
  if (sizeof(T) > LINE_SIZE) {
    throw std::runtime_error("El tipo de dato es más grande que el tamaño de la línea de caché.");
  }

  uint64_t lineAddr = getLineAddr(addr);
  size_t offset = getOffset(addr);

  std::lock_guard<std::mutex> lock(mtx);

  auto it = lines.find(lineAddr);

  // MISS: La línea no está en la caché o es inválida
  if (it == lines.end() || it->second.state == MESIState::INVALID) {
    handleReadMiss(lineAddr);
    return T{};
  }

  // HIT: La línea está en la caché y es válida
  T value;
  memcpy(&value, &it->second.data[offset], sizeof(T));
  return value;
}

template <typename T> void Cache::write(uint64_t addr, const T& value) {
  if (sizeof(T) > LINE_SIZE) {
    throw std::runtime_error("El tipo de dato es más grande que el tamaño de la línea de caché.");
  }

  uint64_t lineAddr = getLineAddr(addr);
  size_t offset = getOffset(addr);

  std::lock_guard<std::mutex> lock(mtx);

  auto it = lines.find(lineAddr);

  // MISS: La línea no está o es inválida
  if (it == lines.end() || it->second.state == MESIState::INVALID) {
    handleWriteMiss(lineAddr);
    return;
  }

  // HIT
  CacheLine& line = it->second;

  // Si está en SHARED, necesitamos obtener propiedad exclusiva
  if (line.state == MESIState::SHARED) {
    line.state = MESIState::MODIFIED; // Transición optimista
    BusPacket upgrade_pkt{BusMsgType::BusUp, lineAddr, id};
    bus->enqueue(id, upgrade_pkt); // Invalida las copias de los demás
  } else                           // EXCLUSIVE o MODIFIED
  {
    line.state = MESIState::MODIFIED;
  }

  // Escribir el dato en la línea de caché
  memcpy(&line.data[offset], &value, sizeof(T));
}

#endif // CACHE_HPP