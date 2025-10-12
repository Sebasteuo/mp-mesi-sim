/*
  Archivo: main.cpp
  Qué hace:
    Orquesta una corrida mínima del simulador con 4 PEs usando memoria simulada.
    Ahora acepta parámetros por línea de comandos para no recompilar cada vez.

  Parámetros admitidos:
    --N <entero>     Tamaño del problema (elementos en A y B)
    --align32        Activa modo de alineamiento de 32 bytes (padding)
    --debug          Imprime info breve por PE (rango y parcial)

  Ejemplos:
    ./build/sim --N 128
    ./build/sim --N 1000 --align32
    ./build/sim --debug
*/

#include <iostream>
#include <thread>
#include <vector>
#include <cmath>
#include <cstring>
#include <string>

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

// Parser mínimo de argumentos: simple y claro
static void parse_args(int argc, char** argv, RunConfig& cfg, bool& debug) {
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--N" && i+1 < argc) {
      cfg.N = std::stoi(argv[++i]);
    } else if (a == "--align32") {
      cfg.align32 = true;
    } else if (a == "--debug") {
      debug = true;
    } else if (a == "--help" || a == "-h") {
      std::cout << "Uso: sim [--N <entero>] [--align32] [--debug]\n";
      std::exit(0);
    } else {
      std::cerr << "Argumento desconocido: " << a << "\n";
      std::cerr << "Prueba con --help\n";
      std::exit(1);
    }
  }
}

int main(int argc, char** argv) {
  RunConfig cfg;
  cfg.N = 32;          // valor por defecto
  cfg.align32 = false;

  bool debug = false;
  parse_args(argc, argv, cfg, debug);

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
    double partial = unpack_double(u);
    if (debug) {
      // Rango aproximado por PE: este print es solo guía rápida
      int base = (cfg.N / 4) * i;
      int rest = cfg.N % 4;
      int extra = (i == 3 ? rest : 0);
      int nloc = (cfg.N / 4) + extra;
      std::cout << "[debug] PE" << i << " rango=[" << base << "," << (base+nloc-1)
                << "] parcial=" << partial << "\n";
    }
    total += partial;
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

  std::cout << "N=" << cfg.N
            << " align32=" << (cfg.align32 ? "on" : "off")
            << "  DotProduct total=" << total
            << " ref=" << ref
            << " |err|=" << err << "\n";
  std::cout << "CSV -> run_metrics.csv\n";
  return 0;
}
