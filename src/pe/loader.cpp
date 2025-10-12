/*
  Archivo: loader.cpp
  Qué hace:
    Implementa un cargador de datos básico para preparar A, B y partial_sums.
  Detalle:
    - Se escribe en memoria usando IDataMem (sin conocer detalles internos).
*/

#include "pe/loader.hpp"
#include <cstring>

// Convierte un double a su representación en 64 bits
static inline uint64_t pack_double(double x) {
  uint64_t u;
  std::memcpy(&u, &x, sizeof(double));
  return u;
}

void Loader::init(IDataMem* m, const RunConfig& c) {
  mem = m;
  cfg = c;
}

// Llena A y B con valores deterministas para poder validar el resultado
void Loader::load_vectors() {
  for (int i = 0; i < cfg.N; ++i) {
    double a = 1.0 + static_cast<double>(i);
    double b = 0.5 * static_cast<double>(i) - 1.0;
    mem->store64(cfg.baseA + i*8, pack_double(a));
    mem->store64(cfg.baseB + i*8, pack_double(b));
  }
}

// Pone en cero los parciales de los cuatro PEs
void Loader::clear_partials() {
  for (int pe = 0; pe < 4; ++pe) {
    mem->store64(cfg.basePartial + pe*8, pack_double(0.0));
  }
}
