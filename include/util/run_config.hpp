#pragma once
#include <cstdint>

/*
  ¿Qué es esto?
  -------------
  Son los parámetros de una corrida: tamaño N, si alineamos a 32B,
  latencias lógicas (sin sleeps), semillas, y direcciones base.
  Con esto hacemos las pruebas reproducibles y fáciles de ajustar.
*/
struct RunConfig {
  int      N = 32;         // tamaño del problema (por ejemplo, elementos por PE)
  bool     align32 = false; // false: 8B natural; true: 32B + padding por PE
  uint64_t seed = 12345;   // semilla fija para reproducibilidad

  // Latencias lógicas (NO se usan sleeps; solo contadores internos)
  int L1_HIT  = 1;
  int BUS_LAT = 10;
  int MEM_LAT = 40;

  // Direcciones base (Harvard simplificada: datos separados del código)
  uint64_t baseA       = 0x0000;
  uint64_t baseB       = 0x4000;
  uint64_t basePartial = 0x8000;
};
