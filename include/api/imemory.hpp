#pragma once
#include <cstdint>
#include "types.hpp"

/*
  ¿Qué hace la memoria?
  ---------------------
  Representa la memoria de datos compartida (tamaño fijo).
  Lee y escribe LÍNEAS COMPLETAS de 32 bytes cuando el bus se lo pide.
*/
struct IMemory {
  virtual ~IMemory() = default;

  // Leer una línea (32B) desde una dirección de línea
  virtual void read_line (uint64_t line_addr, Line32B* out) = 0;

  // Escribir una línea (32B) hacia una dirección de línea (write-back/Flush)
  virtual void write_line(uint64_t line_addr, const Line32B& data) = 0;
};
