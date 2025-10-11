#pragma once
#include <cstdint>
#include "types.hpp"
#include "idata_mem.hpp"

/*
  ¿Qué hace la L1?
  ----------------
  Es la caché privada de cada PE. Implementa la interfaz de datos (IDataMem)
  para que el PE lea/escriba como si fuera memoria. Además, sabe:
  - Hablar con el bus (attach_bus).
  - Recibir respuestas a sus propios pedidos (on_response).
  - Escuchar "snoops" cuando otros PEs piden/invalidan (on_snoop).
*/
struct IBus;

struct ICacheL1 : public IDataMem {
  virtual ~ICacheL1() = default;

  // Conectar esta L1 al bus y asignarle su id
  virtual void attach_bus(IBus* bus, int id) = 0;

  // Respuesta del bus a una solicitud que inició esta L1
  // req_id: correlaciona con el request original
  // msg   : tipo de respuesta (Data, AckInvalidate, etc.)
  // addr  : dirección de la línea
  // data_opt: si hay datos (ej: Data/Flush), llega la línea de 32B
  // shared_hit: true si durante un BusRd se detectaron "sharers" (decide E vs S)
  virtual void on_response(uint32_t req_id, Msg msg, uint64_t addr,
                           const Line32B* data_opt, bool shared_hit) = 0;

  // Snoop: notificación de que OTROS hicieron algo en el bus
  // (ej: Invalidate a esta línea, o BusRd que requiere Flush si estoy en M)
  virtual void on_snoop(Msg msg, uint64_t addr, const Line32B* data_opt) = 0;
};
