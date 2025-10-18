/*
  loader.cpp
  Prepara A, B y limpia partial_sums.
  Si --align32 está activo, los parciales se separan 32 bytes (1 línea de 32B).
*/
#include "pe/loader.hpp"
#include <cstring>

static inline uint64_t pack_double(double x) {
  uint64_t u; std::memcpy(&u, &x, sizeof(double)); return u;
}

void Loader::init(IDataMem* m, const RunConfig& c) {
  mem = m;
  cfg = c;
}

void Loader::load_vectors() {
  // A[i] = 1+i ; B[i] = 0.5*i - 1
  for (int i = 0; i < cfg.N; ++i) {
    double a = 1.0 + i;
    double b = 0.5 * i - 1.0;
    mem->store64(cfg.baseA + i*8, pack_double(a));
    mem->store64(cfg.baseB + i*8, pack_double(b));
  }
}

void Loader::clear_partials() {
  // Si align32, cada PE escribe su parcial separado 32B (evita false sharing).
  const uint64_t stride = cfg.align32 ? 32ull : 8ull;
  for (int pe = 0; pe < 4; ++pe) {
    mem->store64(cfg.basePartial + pe*stride, pack_double(0.0));
  }
}
