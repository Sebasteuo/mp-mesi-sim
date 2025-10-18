/*
  Runner del simulador con dos modos:
    - mock  (por defecto): PEs usan MockRam (32 MiB), útil para N grandes.
    - l1bus: PEs usan su L1, las L1 hablan por el Bus con la memoria compartida.
  Flags:
    --arch mock|l1bus
    --N <entero>
    --align32
    --debug
    --out <csv>
    --seq        (en l1bus: ejecuta los 4 PEs de forma secuencial, no concurrente)
*/

#include <iostream>
#include <thread>
#include <vector>
#include <cmath>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>
#include <array>

#include "api/idata_mem.hpp"
#include "util/run_config.hpp"
#include "util/metrics.hpp"
#include "pe/pe.hpp"
#include "pe/loader.hpp"

// Mock (modo "mock")
extern "C" IDataMem* create_mock_ram();

// L1/Bus/Mem (modo "l1bus")
#include "include/busmem/memory.hpp"
#include "include/busmem/bus.hpp"
#include "include/busmem/messagesTypes.hpp"
#include "include/l1/cache.hpp"
#include "l1/l1_idata_adapter.hpp"

static inline double unpack_double(uint64_t u) { double d; std::memcpy(&d,&u,sizeof(double)); return d; }
static inline uint64_t pack_double(double x)    { uint64_t u; std::memcpy(&u,&x,sizeof(double)); return u; }

// Helpers para escribir 8 bytes en memoria por línea de 32B (API IMemory)
static inline void mem_store64_line(IMemory& mem, uint64_t addr, uint64_t val) {
  uint64_t lineAddr = addr - (addr % LINE_SIZE);
  std::array<uint8_t, LINE_SIZE> line{};
  mem.readLine(lineAddr, line);
  std::memcpy(&line[addr - lineAddr], &val, 8);
  mem.writeLine(lineAddr, line);
}

int main(int argc, char** argv) {
  // -------- parse args --------
  std::string arch = "mock";     // mock | l1bus
  RunConfig cfg; cfg.N = 32; cfg.align32 = false;
  bool debug = false;
  bool seq = false;              // sólo aplica en l1bus
  std::string out_csv = "run_metrics.csv";

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--arch" && i+1 < argc) arch = argv[++i];
    else if (a == "--N" && i+1 < argc) cfg.N = std::stoi(argv[++i]);
    else if (a == "--align32") cfg.align32 = true;
    else if (a == "--debug") debug = true;
    else if (a == "--out" && i+1 < argc) out_csv = argv[++i];
    else if (a == "--seq") seq = true;
  }

  Metrics mx; mx.resize(4);

  if (arch == "mock") {
    // ===================== MODO MOCK =====================
    IDataMem* mem = create_mock_ram();
    cfg.baseA       = 0ull;
    cfg.baseB       = 8ull * 1024 * 1024;
    cfg.basePartial = 16ull * 1024 * 1024;

    Loader loader; loader.init(mem, cfg);
    loader.load_vectors();
    loader.clear_partials();

    PE pes[4];
    for (int i = 0; i < 4; ++i) pes[i].setup(i, mem, cfg, &mx.per_pe[i]);

    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i) ts.emplace_back([&pes,i]{ pes[i].run_kernel(); });
    for (auto& t : ts) t.join();

  } else {
    // ===================== MODO L1BUS =====================
    // Memoria pequeña (4096 B) => usar N pequeño (ej. 32) y bases compactas
    SharedMemory memory;

    // Crear L1s
    std::vector<std::unique_ptr<Cache>> caches;
    std::array<ICache*, L1_COUNT> cache_ptrs{};
    for (int i = 0; i < L1_COUNT; ++i) {
      caches.push_back(std::make_unique<Cache>(i));
      cache_ptrs[i] = caches[i].get();
    }

    // Crear Bus y conectar L1s
    Bus bus(memory, cache_ptrs);
    for (auto& c : caches) c->setBus(bus);

    // Hilo del bus (ellos no expusieron stop; lo dejamos detach)
    std::thread bus_thread(&Bus::run, &bus);
    bus_thread.detach();

    // Bases dentro de 4096 B
    cfg.baseA       = 0ull;     // 0..(N*8-1)
    cfg.baseB       = 512ull;   // separadas 512B
    cfg.basePartial = 1024ull;  // parciales a partir de 1 KiB

    // Precarga directa en memoria (por líneas de 32B)
    for (int i = 0; i < cfg.N; ++i) {
      double a = 1.0 + i;
      double b = 0.5 * i - 1.0;
      mem_store64_line(memory, cfg.baseA + i*8, pack_double(a));
      mem_store64_line(memory, cfg.baseB + i*8, pack_double(b));
    }
    for (int pe = 0; pe < 4; ++pe) {
      mem_store64_line(memory, cfg.basePartial + pe*8, pack_double(0.0));
    }

    // Adaptadores: cada PE verá su L1 como IDataMem
    L1AsIDataMem adp0(caches[0].get());
    L1AsIDataMem adp1(caches[1].get());
    L1AsIDataMem adp2(caches[2].get());
    L1AsIDataMem adp3(caches[3].get());

    PE pes[4];
    pes[0].setup(0, &adp0, cfg, &mx.per_pe[0]);
    pes[1].setup(1, &adp1, cfg, &mx.per_pe[1]);
    pes[2].setup(2, &adp2, cfg, &mx.per_pe[2]);
    pes[3].setup(3, &adp3, cfg, &mx.per_pe[3]);

    if (seq) {
      // Ejecutar PEs uno por uno (evita avalancha mientras L1/Bus no bloquean en miss)
      for (int i = 0; i < 4; ++i) {
        pes[i].run_kernel();
      }
    } else {
      // Ejecutar los 4 PEs en paralelo (esto requiere que L1/Bus manejen bien los misses)
      std::vector<std::thread> ts;
      for (int i = 0; i < 4; ++i) ts.emplace_back([&pes,i]{ pes[i].run_kernel(); });
      for (auto& t : ts) t.join();
    }
  }

  // -------- reducción (sumar resultados por PE) --------
  double total = 0.0;
  for (int i = 0; i < 4; ++i) total += mx.per_pe[i].result;

  // -------- referencia serial --------
  double ref = 0.0;
  for (int i = 0; i < cfg.N; ++i) {
    double a = 1.0 + i;
    double b = 0.5 * i - 1.0;
    ref += a * b;
  }

  // -------- metadatos y CSV --------
  std::time_t t = std::time(nullptr);
  std::ostringstream oss_id;
  oss_id << "ts_" << static_cast<long long>(t)
         << "_N=" << cfg.N
         << "_align=" << (cfg.align32 ? "on" : "off")
         << "_arch=" << arch
         << (seq ? "_seq" : "");
  std::ostringstream oss_cfg;
  oss_cfg << "N=" << cfg.N << ",align32=" << (cfg.align32 ? "on" : "off")
          << ",arch=" << arch << (seq ? ",seq=on" : ",seq=off");
  Metrics mx_out = mx;
  mx.run_id = oss_id.str();
  mx.config_str = oss_cfg.str();

  mx.to_csv(out_csv);

  std::cout << "arch=" << arch
            << " N=" << cfg.N
            << " align32=" << (cfg.align32 ? "on" : "off")
            << (seq ? " (seq)" : "      ")
            << "  DotProduct total=" << total
            << " ref=" << ref
            << " |err|=" << std::abs(total - ref) << "\n";
  std::cout << "CSV -> " << out_csv << "\n";
  return 0;
}
