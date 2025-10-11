# mp-mesi-sim
Multiprocessor MESI simulator (CE-4302).

## Módulos
- Bus + Memoria (Integrante 1)
- Caché L1 (Integrante 2)
- PEs, Loader, Métricas (Sebastián)

## Convenciones del simulador
- Sin sleeps/esperas reales: latencias por **ticks lógicos**.
- Arquitectura **Harvard simplificada** (código y datos separados).
- Colas del Bus con **profundidad finita** (no infinito).
- Mensajes contados: **control** (1–2 B) vs **datos** (líneas de 32 B).
