/*
  Archivo: ram_mock.cpp
  Qué es:
    Memoria plana muy simple que implementa la interfaz IDataMem.
  Para qué sirve:
    Permite ejecutar y probar el módulo de PEs, Loader y Métricas
    aunque la caché L1 y el Bus/Memoria no estén listos todavía.
  Cómo funciona:
    Guarda los bytes en un vector interno (1 MiB). Los loads/stores de 64 bits
    leen y escriben directamente en ese arreglo. No hay coherencia ni latencias,
    es solo un respaldo de memoria lineal para comenzar a correr el programa.
*/

#include <vector>
#include <cstdint>
#include <cstring>
#include "api/idata_mem.hpp"

class MockRam : public IDataMem {
 public:
  MockRam() { ram.resize(1<<20, 0); }  // Reserva 1 MiB

  // Lee 8 bytes desde la dirección 'addr'
  uint64_t load64(uint64_t addr) override {
    uint64_t v = 0;
    std::memcpy(&v, &ram[addr], 8);
    return v;
  }

  // Escribe 8 bytes en la dirección 'addr'
  void store64(uint64_t addr, uint64_t val) override {
    std::memcpy(&ram[addr], &val, 8);
  }

 private:
  // Arreglo de bytes que simula la memoria principal
  std::vector<uint8_t> ram;
};

// Fábrica en C para que el main pueda crear una instancia fácilmente
extern "C" IDataMem* create_mock_ram() { return new MockRam(); }
