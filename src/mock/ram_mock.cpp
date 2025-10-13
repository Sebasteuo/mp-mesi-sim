/*
  Archivo: ram_mock.cpp
  Memoria plana simple que implementa IDataMem.
  Se usa para correr el runner sin depender de L1/Bus reales.
  Nota: 32 MiB para soportar N grandes sin pisarse A/B/partials.
*/
#include <vector>
#include <cstdint>
#include <cstring>
#include "api/idata_mem.hpp"

class MockRam : public IDataMem {
 public:
  MockRam() { ram.resize(32u << 20, 0); }  // 32 MiB

  uint64_t load64(uint64_t addr) override {
    uint64_t v = 0;
    std::memcpy(&v, &ram.at(static_cast<size_t>(addr)), 8);
    return v;
  }
  void store64(uint64_t addr, uint64_t val) override {
    std::memcpy(&ram.at(static_cast<size_t>(addr)), &val, 8);
  }

 private:
  std::vector<uint8_t> ram;
};

extern "C" IDataMem* create_mock_ram() { return new MockRam(); }
