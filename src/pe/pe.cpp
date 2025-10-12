/*
  Archivo: pe.cpp
  Qué hace:
    Implementa el núcleo (PE) que ejecuta su parte del dot product.
  Detalle:
    - El PE usa IDataMem para leer A y B y escribir su parcial.
    - No conoce cachés ni bus. Es intencional para separar responsabilidades.
*/

#include "pe/pe.hpp"
#include <cstring>

// Convierte 64 bits a double sin reinterpret_cast arriesgados
static inline double unpack_double(uint64_t u) {
  double d;
  std::memcpy(&d, &u, sizeof(double));
  return d;
}
static inline uint64_t pack_double(double x) {
  uint64_t u;
  std::memcpy(&u, &x, sizeof(double));
  return u;
}

void PE::setup(int id_, IDataMem* mem_, const RunConfig& cfg_, PeMetrics* slot) {
  id  = id_;
  mem = mem_;
  cfg = cfg_;
  mx  = slot;

  // División simple del trabajo entre 4 PEs
  int base = (cfg.N / 4) * id;
  int rest = cfg.N % 4;
  int extra = (id == 3 ? rest : 0);

  N_local = (cfg.N / 4) + extra;

  // Bases para el segmento de A y B que toca a este PE
  baseA = cfg.baseA + base * 8;
  baseB = cfg.baseB + base * 8;

  // Los parciales se guardan en basePartial con un offset por id
  basePartial = cfg.basePartial;
}

void PE::run_kernel() {
  double sum = 0.0;

  // Recorre su segmento y acumula A[i] * B[i]
  for (int i = 0; i < N_local; ++i) {
    uint64_t ua = mem->load64(baseA + i*8);  mx->loads++;
    uint64_t ub = mem->load64(baseB + i*8);  mx->loads++;

    double a = unpack_double(ua);
    double b = unpack_double(ub);
    sum += a * b;
  }

  // Escribe su parcial y registra el resultado
  mem->store64(basePartial + id*8, pack_double(sum)); mx->stores++;
  mx->result = sum;
}
