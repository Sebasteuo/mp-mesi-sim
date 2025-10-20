#ifndef CACHE_METRICS_HPP
#define CACHE_METRICS_HPP

#include <cstdint>

struct CacheMetrics {
  uint64_t hits = 0;       // Aciertos
  uint64_t misses_r = 0;   // Fallos de lectura
  uint64_t misses_w = 0;   // Fallos de escritura (incluye I->E y S->E)
  uint64_t inval_sent = 0; // Invalidaciones enviadas (por BusUp)
  uint64_t inval_recv = 0; // Invalidaciones recibidas (en snoop)
  uint64_t busy_ticks = 0; // Ciclos ocupada (procesando hits)
};

#endif // CACHE_METRICS_HPP