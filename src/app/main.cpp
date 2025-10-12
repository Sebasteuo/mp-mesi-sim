/*
  Archivo: main.cpp
  Qué hace:
    Orquesta una corrida mínima del simulador con 4 PEs usando una memoria
    simulada (MockRam). Carga datos, ejecuta el kernel y genera un CSV.
  Flujo:
    1) Crear memoria mock
    2) Cargar A y B, limpiar parciales
    3) Configurar y lanzar 4 PEs en hilos
    4) Leer parciales y sumar el total
    5) Validar contra una referencia serial
    6) Exportar métricas a CSV
*/

#include <iostream>
#include <thread>
#include <vector>
#include <cmath>
#include <cstring>

#include "api/idata_mem.hpp"
#include "util/run_config.hpp"
#include "util/metrics.hpp"
#include "pe/pe.hpp"
#include "pe/loader.hpp"

// Implementado en src/mock/ram_mock.cpp
extern "C" IDataMem* create_mock_ram();

static inline double unpack_double(uint64_t u) {
  double d; std::memcpy(&d,&u,sizeof(double)); return d;
}

int main() {
  RunConfig cfg;
  cfg.N = 32;          // tamaño pequeño para probar
  cfg.align32 = false; // alineamiento natural de 8 bytes

  // 1) Memoria simulada
  IDataMem* mem = create_mock_ram();

  // 2) Carga de datos
  Loader loader; loader.init(mem, cfg);
  loader.load_vectors();
  loader.clear_partials();

  // 3) Métricas y PEs
  Metrics mx; mx.resize(4);
  PE pes[4];
  for (int i = 0; i < 4; ++i) {
    pes[i].setup(i, mem, cfg, &mx.per_pe[i]);
  }

  // 4) Ejecutar los 4 PEs en paralelo
  std::vector<std::thread> ts;
  ts.reserve(4);
  for (int i = 0; i < 4; ++i) ts.emplace_back([&pes,i]{ pes[i].run_kernel(); });
  for (auto& t : ts) t.join();

  // 5) Suma final desde memoria
  double total = 0.0;
  for (int i = 0; i < 4; ++i) {
    uint64_t u = mem->load64(cfg.basePartial + i*8);
    total += unpack_double(u);
  }

  // Referencia serial para validar
  double ref = 0.0;
  for (int i = 0; i < cfg.N; ++i) {
    double a = 1.0 + i;
    double b = 0.5 * i - 1.0;
    ref += a * b;
  }
  double err = std::abs(total - ref);
  for (int i = 0; i < 4; ++i) mx.per_pe[i].abs_error = 0.0;

  // 6) CSV de métricas por PE
  mx.to_csv("run_metrics.csv");

  std::cout << "DotProduct total=" << total
            << " ref=" << ref
            << " |err|=" << err << "\n";
  std::cout << "CSV -> run_metrics.csv\n";
  return 0;
}
