#pragma once
#include <cstdint>
#include "api/idata_mem.hpp"
#include "include/l1/cache.hpp"  // Cache (L1) de Randall

// Adaptador que permite usar Cache (L1) a través de la interfaz IDataMem.
// Así los PEs no cambian: siguen viendo "memoria", pero en realidad es su L1.
class L1AsIDataMem : public IDataMem {
 public:
  explicit L1AsIDataMem(Cache* c) : l1_(c) {}

  uint64_t load64(uint64_t addr) override {
    // Lee 8 bytes desde la L1
    return l1_->read<uint64_t>(addr);
  }

  void store64(uint64_t addr, uint64_t val) override {
    // Escribe 8 bytes en la L1
    l1_->write<uint64_t>(addr, val);
  }

 private:
    Cache* l1_;
};
