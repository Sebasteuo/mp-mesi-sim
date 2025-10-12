#pragma once
/*
  Archivo: pe.hpp
  Qué es:
    Definición del "Processing Element" (PE), que realiza el cómputo del dot product.
  Para qué sirve:
    Ejecuta su parte del trabajo sobre A y B, y guarda su suma parcial en memoria.
  Cómo funciona:
    El PE solo usa la interfaz IDataMem (no conoce cachés ni bus). Lee A y B, acumula,
    y escribe el resultado parcial en partial_sums[id].
*/

#include <cstdint>
#include "api/idata_mem.hpp"
#include "util/metrics.hpp"
#include "util/run_config.hpp"

struct PE {
  int       id = 0;          // Identificador del PE (0..3)
  IDataMem* mem = nullptr;   // Memoria vista por el PE
  RunConfig cfg{};           // Parámetros de ejecución
  PeMetrics* mx = nullptr;   // Dónde acumular métricas de este PE

  // Direcciones base del segmento que le toca a este PE
  uint64_t baseA = 0, baseB = 0, basePartial = 0;
  int      N_local = 0;      // Elementos que procesa este PE

  // Configura el PE con su id, la memoria a usar, la configuración y su slot de métricas
  void setup(int id_, IDataMem* mem_, const RunConfig& cfg_, PeMetrics* slot);

  // Ejecuta el kernel del dot product sobre su segmento y escribe su parcial
  void run_kernel();
};
