#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <chrono>

#include "include/busmem/memory.hpp"
#include "include/busmem/bus.hpp"
#include "include/l1/cache.hpp"
#include "include/busmem/messagesTypes.hpp"

static inline uint64_t pack_double(double x) {
  uint64_t u;
  std::memcpy(&u, &x, 8);
  return u;
}
static inline double unpack_double(uint64_t u) {
  double d;
  std::memcpy(&d, &u, 8);
  return d;
}

int main() {
  // 1) Memoria y precarga de una línea conocida (addrLine = 0)
  SharedMemory mem;
  std::array<uint8_t, LINE_SIZE> line{};
  double val = 42.5;
  std::memcpy(&line[0], &val, 8);
  mem.writeLine(0, line);

  // 2) Crear 4 L1 como espera el Bus
  std::array<ICache*, L1_COUNT> cache_ptrs{};
  std::vector<std::unique_ptr<Cache>> caches;
  caches.reserve(L1_COUNT);
  for (int i = 0; i < L1_COUNT; ++i) {
    caches.push_back(std::make_unique<Cache>(i));
    cache_ptrs[i] = caches[i].get();
  }

  // 3) Bus en heap (para evitar destrucción al salir, ya que el hilo sigue vivo)
  Bus* bus = new Bus(mem, cache_ptrs);
  for (auto& c : caches)
    c->setBus(*bus);

  std::thread tb(&Bus::run, bus);
  tb.detach(); // no hay stop() público; lo dejamos vivo hasta terminar el proceso

  // 4) Lecturas: si L1 bloquea en miss, ambas deben ver 42.5
  double r0 = caches[0]->read<double>(0);
  double r1 = caches[1]->read<double>(0);
  if (r0 != 42.5 || r1 != 42.5) {
    std::cerr << "FAIL: r0=" << r0 << " r1=" << r1 << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return 1;
  }

  // 5) Escritura y coherencia: c0 escribe 100.0; c1 debe leer 100.0
  caches[0]->write<double>(0, 100.0);
  double r1b = caches[1]->read<double>(0);
  if (r1b != 100.0) {
    std::cerr << "FAIL: r1 tras write no es 100.0, r1b=" << r1b << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return 2;
  }

  std::cout << "OK: L1 miss bloqueante + coherencia básica funcionan.\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return 0;
}
