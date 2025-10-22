#pragma once
/*
  Archivo: loader.hpp
  Qué es:
    Un ayudante para preparar la memoria antes de ejecutar el kernel.
  Para qué sirve:
    - Llena los arreglos A[] y B[] con valores deterministas.
    - Limpia los cuatro partial_sums (uno por cada PE).
  Cómo funciona:
    Usa la interfaz IDataMem para escribir valores de 64 bits en memoria.
*/

#include <cstdint>
#include "api/idata_mem.hpp"
#include "busmem/memory_map.hpp" 
#include "util/run_config.hpp"

struct Loader {
  IDataMem* mem = nullptr;  // Vista de memoria (mock ahora, L1 real después)
  RunConfig cfg{};          // Parámetros de ejecución
  MemLayout layout{};

  // Inicializa el loader con la memoria y la configuración a usar
  void init(IDataMem* m, const RunConfig& c);

  // Configura el layout de la memoria
  void setup_layout(std::uint32_t N, std::uint32_t hw, std::uint32_t pes, bool align32);

  // Para escribir posibles constantes
  void write_consts();

  // Carga A[i] = 1 + i y B[i] = 0.5*i - 1 en las direcciones configuradas
  void load_vectors();

  // Pone en cero partial_sums[0..3]
  void clear_partials();
};
