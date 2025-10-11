#pragma once
#include <cstdint>

/*
  ¿Para qué sirve esto?
  ---------------------
  Si luego queremos ejecutar el kernel como "programa" (ISA mínima),
  necesitamos una memoria de instrucciones. Esta interfaz es simple:
  fetch64(PC) devuelve 8 bytes que representan la instrucción en esa dirección.
  Por ahora es opcional, pero nos deja listos para cumplir "carga de código".
*/
struct IInstrMem {
  virtual ~IInstrMem() = default;
  virtual uint64_t fetch64(uint64_t pc) = 0; // Traer 8 bytes de instrucción
};
