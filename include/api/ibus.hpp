#pragma once
#include <cstdint>
#include "types.hpp"

/*
  ¿Qué hace el Bus?
  -----------------
  Es el árbitro y mensajero del sistema. Recibe pedidos de las L1,
  decide a quién atiende (round-robin con colas finitas), difunde "snoops"
  y entrega respuestas (Data/Ack) a la L1 que inició la transacción.
*/
struct ICacheL1;

struct IBus {
  virtual ~IBus() = default;

  // Registrar una caché L1 por id (0..3)
  virtual void register_l1(int id, ICacheL1* l1) = 0;

  // Encolar una solicitud desde una L1.
  // req_id: número para reconocer la respuesta correcta después.
  // msg   : tipo de mensaje (BusRd, BusUp, etc.)
  // addr  : dirección de la línea/byte involucrado
  // src_id: quién hizo la solicitud (qué PE/L1)
  virtual void request(uint32_t req_id, Msg msg, uint64_t addr, int src_id) = 0;

  // Avanzar el simulador un "paso" lógico del bus (procesa 1 transacción).
  // Importante: NO usa sleeps. Todo es por ticks lógicos.
  virtual void tick() = 0;
};
