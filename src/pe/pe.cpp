/*
  pe.cpp
  Núcleo del PE: lee A y B, acumula y escribe su parcial.
  El parcial se escribe con stride 8B o 32B según --align32.
*/
#include "pe/pe.hpp"
#include <cstring>

static inline double unpack_double(uint64_t u) {
  double d; std::memcpy(&d, &u, sizeof(double)); return d;
}
static inline uint64_t pack_double(double x) {
  uint64_t u; std::memcpy(&u, &x, sizeof(double)); return u;
}

void PE::setup(int pe_id, IDataMem* m, const RunConfig& c, PeMetrics* slot) {
  id = pe_id;
  mem = m;
  cfg = c;
  mx = slot;
}

void PE::run_kernel() {
  // División simple en 4 segmentos; el último toma el residuo.
  int base = (cfg.N / 4) * id;
  int rest = cfg.N % 4;
  int extra = (id == 3 ? rest : 0);
  int nloc = (cfg.N / 4) + extra;

  double sum = 0.0;
  for (int i = 0; i < nloc; ++i) {
    uint64_t a_u = mem->load64(cfg.baseA + (base + i)*8);
    uint64_t b_u = mem->load64(cfg.baseB + (base + i)*8);
    double a = unpack_double(a_u);
    double b = unpack_double(b_u);
    sum += a * b;
    if (mx) mx->loads += 2;
  }

  const uint64_t stride = cfg.align32 ? 32ull : 8ull;
  mem->store64(cfg.basePartial + id*stride, pack_double(sum));
  if (mx) { mx->stores += 1; mx->result = sum; }
}
