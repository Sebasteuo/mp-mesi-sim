#pragma once
#include <cstdint>

/*
  ¿Qué es este archivo?
  ----------------------
  Aquí definimos tipos MUY básicos que todo el proyecto usa:
  - El estado MESI de una línea de caché (I, S, E, M).
  - Los tipos de mensajes que viajan por el bus (lecturas, invalidaciones, etc.).
  - El formato de "línea de caché" de 32 bytes.
  - Un ayudante para partir una dirección en tag / índice / offset.

  Lenguaje simple:
  - MESI nos dice si una línea está inválida, compartida, exclusiva o modificada.
  - Los mensajes del bus son "señales" que se mandan entre cachés/memoria.
  - Una "línea" es el bloque fijo de datos que movemos (aquí siempre 32 bytes).
*/

enum class Mesi : uint8_t { I=0, S=1, E=2, M=3 };

enum class Msg : uint8_t {
  BusRd,        // Leer línea (otra caché o memoria debe responder)
  BusUp,        // Pedir permiso para escribir (S->M), invalidando a otros
  Invalidate,   // Aviso para invalidar esta línea en otras cachés
  Flush,        // Enviar una línea modificada (dirty) hacia el bus/memoria
  Data,         // Respuesta con la línea completa (32B)
  AckInvalidate // Confirmación de que todos invalidaron
};

// Una línea fija de 32 bytes (el "bloque" que viaja por el sistema)
struct Line32B { uint8_t b[32]; };

// Ayudantes para separar una dirección en partes (dado que la línea es de 32B):
// - offset: 5 bits  (dentro de la línea: 0..31)
// - index : 3 bits  (8 sets, porque hay 16 líneas 2-way => 8 conjuntos)
// - tag   : el resto
struct Address {
  static inline uint64_t tag   (uint64_t a) { return a >> 8; }            // [63:8]
  static inline uint32_t index (uint64_t a) { return (a >> 5) & 0x7; }    // [7:5]
  static inline uint32_t offset(uint64_t a) { return  a        & 0x1F; }  // [4:0]
};
