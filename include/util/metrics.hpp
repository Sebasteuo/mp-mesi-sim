#pragma once
/*
  Archivo: metrics.hpp
  Qué es:
    Estructuras para guardar métricas por PE y para escribir un CSV con resultados.
  Para qué sirve:
    Permite registrar accesos, contadores y resultados de cada PE y exportarlos
    a un archivo para analizar el comportamiento del simulador.
  Cómo funciona:
    - PeMetrics guarda contadores y el resultado del parcial de cada PE.
    - Metrics es un contenedor de PeMetrics con funciones de ayuda.
*/

#include <cstdint>
#include <string>
#include <vector>

struct PeMetrics {
  int id = 0;                    // Identificador del PE (0..3)

  // Contadores relacionados con caché/coherencia (se llenarán al integrar L1/Bus)
  uint64_t hits = 0, misses_r = 0, misses_w = 0;
  uint64_t inval_sent = 0, inval_recv = 0;

  // Accesos de alto nivel del PE (sí podemos contarlos desde ya)
  uint64_t loads = 0, stores = 0;

  // Estimación de tráfico a nivel de bus (se llenará al integrar)
  uint64_t bus_msgs_ctrl = 0, bus_bytes_ctrl = 0;
  uint64_t bus_lines_data = 0, bus_bytes_data = 0;

  // Tiempo lógico acumulado (cuando integremos con L1/Bus)
  uint64_t ticks = 0;

  // Resultado del parcial y error absoluto usado en validación
  double   result = 0.0;
  double   abs_error = 0.0;
};

struct Metrics {
  std::vector<PeMetrics> per_pe; // Un slot por cada PE

  // Ajusta el tamaño del vector según la cantidad de PEs
  void resize(int pes);

  // Escribe un archivo CSV con una fila por PE
  void to_csv(const std::string& path) const;
};
