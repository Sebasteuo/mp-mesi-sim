/*
  Archivo: main.cpp
  Corre una simulación mínima con 4 PEs usando memoria simulada (MockRam).
  Carga datos, ejecuta el kernel, valida contra referencia y escribe un CSV.
  Parámetros:
    --N <entero>      tamaño del problema
    --align32         activa modo 32B (se usará al integrar padding)
    --debug           imprime rango y parcial por PE
    --out <archivo>   nombre del CSV de salida
*/
#include <iostream>
#include <thread>
#include <vector>
#include <cmath>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

#include "api/idata_mem.hpp"
#include "util/run_config.hpp"
#include "util/metrics.hpp"
#include "pe/pe.hpp"
#include "pe/loader.hpp"

extern "C" IDataMem* create_mock_ram();

static inline double unpack_double(uint64_t u) {
  double d; std::memcpy(&d,&u,sizeof(double)); return d;
}

int main(int argc, char** argv) {
  RunConfig cfg;
  cfg.N = 32;
  cfg.align32 = false;
  bool debug = false;
  std::string out_csv = "run_metrics.csv";

  // Parámetros
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--N" && i+1 < argc) cfg.N = std::stoi(argv[++i]);
    else if (a == "--align32") cfg.align32 = true;
    else if (a == "--debug") debug = true;
    else if (a == "--out" && i+1 < argc) out_csv = argv[++i];
  }

  // Bases separadas para evitar solapes con N grande
  cfg.baseA        = 0ull;               // A desde 0
  cfg.baseB        = 8ull * 1024 * 1024; // B a +8 MiB
  cfg.basePartial  = 16ull * 1024 * 1024;// parciales a +16 MiB

  // Memoria simulada
  IDataMem* mem = create_mock_ram();

  // Carga de datos
  Loader loader; loader.init(mem, cfg);
  loader.load_vectors();
  loader.clear_partials();

  // Métricas y PEs
  Metrics mx; mx.resize(4);
  PE pes[4];
  for (int i = 0; i < 4; ++i) {
    pes[i].setup(i, mem, cfg, &mx.per_pe[i]);
  }

  // Ejecutar en 4 hilos
  std::vector<std::thread> ts;
  for (int i = 0; i < 4; ++i) ts.emplace_back([&pes,i]{ pes[i].run_kernel(); });
  for (auto& t : ts) t.join();

  // Suma total desde parciales
  double total = 0.0;
  for (int i = 0; i < 4; ++i) {
    uint64_t u = mem->load64(cfg.basePartial + i*8);
    double partial = unpack_double(u);

    if (debug) {
      int base = (cfg.N / 4) * i;
      int rest = cfg.N % 4;
      int extra = (i == 3 ? rest : 0);
      int nloc = (cfg.N / 4) + extra;
      std::cout << "[debug] PE" << i
                << " rango=[" << base << "," << (base+nloc-1)
                << "] parcial=" << partial << "\n";
    }

    total += partial;
  }

  // Referencia serial
  double ref = 0.0;
  for (int i = 0; i < cfg.N; ++i) {
    double a = 1.0 + i;
    double b = 0.5 * i - 1.0;
    ref += a * b;
  }

  // Metadatos de corrida
  std::time_t t = std::time(nullptr);
  std::ostringstream oss_id;
  oss_id << "ts_" << static_cast<long long>(t)
         << "_N=" << cfg.N
         << "_align=" << (cfg.align32 ? "on" : "off");
  std::ostringstream oss_cfg;
  oss_cfg << "N=" << cfg.N << ",align32=" << (cfg.align32 ? "on" : "off");
  mx.run_id = oss_id.str();
  mx.config_str = oss_cfg.str();

  // CSV
  mx.to_csv(out_csv);

  std::cout << "N=" << cfg.N
            << " align32=" << (cfg.align32 ? "on" : "off")
            << "  DotProduct total=" << total
            << " ref=" << ref
            << " |err|=" << std::abs(total - ref) << "\n";
  std::cout << "CSV -> " << out_csv << "\n";
  return 0;
}
