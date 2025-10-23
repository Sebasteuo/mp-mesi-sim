/*
  loader.cpp
  Prepara A, B y limpia partial_sums usando SIEMPRE las bases de RunConfig.
  - A[i] = 1 + i
  - B[i] = 0.5*i - 1
  - Parciales[pe] = 0.0

  Nota: Solo si baseA/baseB/basePartial vienen TODOS en 0, se aplica un layout implícito
  (A=0, B justo después de A, Parciales después de B), respetando --align32.
*/
#include "pe/loader.hpp"
#include <cstring>
#include <cstdint>
#include <iostream>

static inline uint64_t pack_double(double x){
  uint64_t u; std::memcpy(&u,&x,sizeof(double)); return u;
}

void Loader::init(IDataMem* m, const RunConfig& c){
  mem = m; cfg = c;

  // Fallback de segmentación SOLO si NO nos pasaron layout (todas en 0).
  if (cfg.baseA==0 && cfg.baseB==0 && cfg.basePartial==0) {
    auto align_up = [&](uint64_t v, uint64_t a){ return (a==0)? v : ((v + a - 1) / a) * a; };
    uint64_t bytes = static_cast<uint64_t>(cfg.N) * 8ull;  // N doubles (8B c/u)
    uint64_t A0 = 0;
    uint64_t B0 = align_up(A0 + bytes, cfg.align32 ? 32ull : 1ull);
    uint64_t P0 = align_up(B0 + bytes, cfg.align32 ? 32ull : 1ull);
    cfg.baseA = A0; cfg.baseB = B0; cfg.basePartial = P0;
    std::cerr << "[INFO] Loader: usando layout implícito A="<<A0<<" B="<<B0<<" P="<<P0
              << " (N="<<cfg.N<<", align32="<<(cfg.align32? "on":"off")<<")\n";
  }
}

void Loader::load_vectors(){
  // Escribe A y B contiguos (double = 8B) en las bases de cfg
  for (int i = 0; i < cfg.N; ++i) {
    const uint64_t off = static_cast<uint64_t>(i) * 8ull;
    const double a = 1.0 + i;
    const double b = 0.5*i - 1.0;
    mem->store64(cfg.baseA + off, pack_double(a));
    mem->store64(cfg.baseB + off, pack_double(b));
  }
}

void Loader::clear_partials(){
  // Parciales en 0, con stride 8B o 32B según --align32
  const uint64_t stride = cfg.align32 ? 32ull : 8ull;
  for (int pe = 0; pe < 4; ++pe) {
    mem->store64(cfg.basePartial + static_cast<uint64_t>(pe) * stride, pack_double(0.0));
  }
}
