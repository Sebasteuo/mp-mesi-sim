#include <vector>
#include <cstdint>
#include <cstring>
#include "api/idata_mem.hpp"

/*
  MockRam (explicación en simple):
  - Es una RAM plana que cumple la interfaz de datos que usa el PE (IDataMem).
  - Sirve para que Sebastián pruebe su kernel y métricas desde YA,
    aunque la L1 y el Bus/Memoria (Randall y José) no estén listos todavía.
  - Luego, reemplazaremos esta RAM por la L1 real conectada al Bus.
*/
class MockRam : public IDataMem {
 public:
  MockRam() { ram.resize(1<<20, 0); } // 1 MiB para jugar tranquilos
  uint64_t load64(uint64_t addr) override {
    uint64_t v=0; std::memcpy(&v, &ram[addr], 8); return v;
  }
  void store64(uint64_t addr, uint64_t val) override {
    std::memcpy(&ram[addr], &val, 8);
  }
 private:
  std::vector<uint8_t> ram;
};

// Nota: luego en el runner/PE podemos crear MockRam y usarla como IDataMem.
