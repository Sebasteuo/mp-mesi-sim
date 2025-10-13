#pragma once
/*
  Archivo: metrics.hpp
  Estructuras para guardar métricas por PE y exportarlas a CSV.
  Incluye metadatos de la corrida (run_id, config_str) repetidos por fila
  para facilitar filtrado y análisis posteriores.
*/
#include <cstdint>
#include <string>
#include <vector>

struct PeMetrics {
  int id = 0;  // 0..3

  // Se llenarán al integrar caché/bus
  uint64_t hits = 0, misses_r = 0, misses_w = 0;
  uint64_t inval_sent = 0, inval_recv = 0;

  // Accesos de alto nivel del PE (ya medibles)
  uint64_t loads = 0, stores = 0;

  // Tráfico de bus (cuando se integre)
  uint64_t bus_msgs_ctrl = 0, bus_bytes_ctrl = 0;
  uint64_t bus_lines_data = 0, bus_bytes_data = 0;

  // Tiempo lógico (cuando se integre)
  uint64_t ticks = 0;

  // Resultado parcial y error
  double result = 0.0;
  double abs_error = 0.0;
};

struct Metrics {
  // Metadatos de corrida
  std::string run_id;     // ej: ts_1697130000_N=32_align=off
  std::string config_str; // ej: N=32,align32=off

  std::vector<PeMetrics> per_pe;

  void resize(int pes);
  void to_csv(const std::string& path) const;
};
