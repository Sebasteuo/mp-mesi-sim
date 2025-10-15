#ifndef MESSAGES_HPP
#define MESSAGES_HPP

#include "messagesTypes.hpp"
#include <array>
#include <cstdint>

// Solicitud/paquete de bus
struct BusPacket {
  BusMsgType type;
  uint64_t addrLine;                     // dirección alineada al tamaño de línea
  int src;                               // L1 origen
  std::array<uint8_t, LINE_SIZE> data{}; // útil para Flush/Data
};

// Respuesta de snoop por cada L1
struct SnoopResp {
  bool hasLine = false;    // ¿posee la línea?
  bool isModified = false; // ¿en M?
  bool sharedHit = false;  // ¿compartida?
  std::array<uint8_t, LINE_SIZE> data{};
};

#endif // MESSAGES_HPP