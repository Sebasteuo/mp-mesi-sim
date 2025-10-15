#ifndef BUS_METRICS_HPP
#define BUS_METRICS_HPP

#include "messages.hpp"
#include "messagesTypes.hpp"
#include <cstdint>

struct BusMetrics {
  uint64_t msg_count = 0;          // # mensajes (control + data)
  uint64_t bytes_total = 0;        // totales (control + datos)
  uint64_t data_payload_bytes = 0; // solo DATA (múltiplos de 32B)
  // desglose por tipo:
  uint64_t busrd = 0, busup = 0, inv = 0, flush = 0, data = 0;

  void add(const BusPacket& p) {
    msg_count++;
    switch (p.type) {
    case BusMsgType::BusRd:
      busrd++;
      break;
    case BusMsgType::BusUp:
      busup++;
      break;
    case BusMsgType::Invalidate:
      inv++;
      break;
    case BusMsgType::Flush:
      flush++;
      break;
    case BusMsgType::Data:
      data++;
      break;
    }
    // Modelo simple: 8 bytes por encabezado + payload si aplica
    bytes_total += 8;
    if (p.type == BusMsgType::Flush || p.type == BusMsgType::Data) {
      bytes_total += LINE_SIZE;
      data_payload_bytes += LINE_SIZE;
    }
  }
};

#endif // BUS_METRICS_HPP