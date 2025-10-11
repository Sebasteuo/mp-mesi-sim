#pragma once
#include <cstdint>

/*
  ¿Qué es esta interfaz?
  ----------------------
  Es la "ventana" que los procesadores (PEs) usan para leer/escribir datos.
  Importante: los PEs NO saben si detrás hay una caché, un bus, etc.
  Solo ven load64 y store64. Así mantenemos el diseño simple y limpio.
*/
struct IDataMem {
  virtual ~IDataMem() = default;

  // Leer 8 bytes (64 bits) desde una dirección
  virtual uint64_t load64 (uint64_t addr) = 0;

  // Escribir 8 bytes (64 bits) en una dirección
  virtual void     store64(uint64_t addr, uint64_t val) = 0;
};
