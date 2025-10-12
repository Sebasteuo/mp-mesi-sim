# mp-mesi-sim

Simulador de un sistema multiprocesador con coherencia de caché mediante el protocolo MESI, desarrollado para el curso CE-4302 Arquitectura de Computadores II.

## Objetivo

Construir un simulador en C++17 con cuatro núcleos (PEs), cada uno con su propia caché L1 privada y una memoria compartida coherente. El simulador ejecuta un kernel de producto punto (dot product) y permite analizar cómo afectan la coherencia, las latencias lógicas y el tráfico de bus al rendimiento.

## Estructura general del repositorio

```text
mp-mesi-sim/
├── include/               -> Archivos de cabecera e interfaces comunes
│   ├── api/               -> Contratos base: IDataMem, ICacheL1, IBus, IMemory
│   ├── pe/                -> Código del módulo de los PEs y Loader
│   └── util/              -> Configuración y métricas
├── src/                   -> Implementaciones
│   ├── pe/                -> Procesadores, Loader y métricas (Sebastián)
│   ├── l1/                -> Caché L1 (Randall)
│   ├── busmem/            -> Bus y Memoria (José)
│   ├── mock/              -> Implementaciones temporales de prueba
│   └── app/               -> Punto de entrada principal (main.cpp)
├── docs/                  -> Avances, diagramas y especificación del proyecto
├── scripts/               -> Scripts para compilar, ejecutar y formatear
├── tests/                 -> Casos de prueba e integración
└── CMakeLists.txt         -> Archivo principal de compilación
```

## Asignación de módulos

| Integrante | Módulo | Descripción |
|-------------|---------|-------------|
| Sebastián | PEs, Loader y Métricas | Ejecuta el kernel, recolecta estadísticas y genera el archivo CSV |
| Randall | Caché L1 | Controla la coherencia con MESI, maneja lecturas, escrituras y reemplazos |
| José | Bus y Memoria | Controla la comunicación entre las L1 y la memoria compartida |

## Convenciones del simulador

- No se utilizan esperas reales ni sleeps. Todas las latencias se representan con tiempo lógico.
- Se usa una arquitectura tipo Harvard simplificada (separación entre código y datos).
- El bus tiene colas con profundidad finita para simular contención real.
- Se contabilizan los mensajes:
  - Mensajes de control (1 a 2 bytes)
  - Mensajes de datos (líneas completas de 32 bytes)

## Cómo compilar y ejecutar

# Construir el proyecto
./scripts/build.sh

# Ejecutar una demo simple
./scripts/run_demo.sh

Por defecto se compila con una memoria simulada (MockRam) que permite ejecutar el proyecto aunque los módulos L1 y Bus+Memoria todavía no estén implementados.

## Estructura modular del simulador

Cada PE utiliza una interfaz de memoria (IDataMem) sin conocer lo que hay detrás. Esa interfaz puede estar conectada a una caché L1, que a su vez se comunica con el bus y la memoria.  
La coherencia se mantiene mediante el protocolo MESI.

PE[i] -> L1[i] <-> Bus <-> Memoria

Flujo de una operación típica:
1. El PE ejecuta un load64 o store64.
2. La L1 verifica si hay hit o miss.
3. Si hay miss, la L1 pide la línea al bus.
4. El bus arbitra y consulta al resto de cachés o a la memoria.
5. La memoria o una caché responde con la línea.
6. Se actualizan los contadores y métricas correspondientes.

## Estado actual del proyecto

- Estructura base del repositorio lista
- Sistema de compilación con CMake funcionando
- CI automático en GitHub Actions configurado
- Mocks activos para ejecutar el proyecto sin dependencias
- Interfaces base y contratos definidos
- Pendiente la implementación de los módulos L1, Bus/Mem y PE

## Scripts disponibles

| Script | Descripción |
|---------|-------------|
| scripts/build.sh | Compila el proyecto |
| scripts/run_demo.sh | Ejecuta la demo con los mocks |
| scripts/format.sh | Aplica formato al código con clang-format |

## Integración con GitHub

- Cada push o Pull Request ejecuta automáticamente el CI.
- CODEOWNERS asigna revisores automáticamente:
  - /src/l1/ -> @Hack998
  - /src/busmem/ -> @jcur02
  - /src/pe/ -> @Sebasteuo

## Créditos

Proyecto académico para el curso CE-4302 Arquitectura de Computadores II  
Instituto Tecnológico de Costa Rica (TEC)  
Año 2025  
Integrantes: Sebastián, Randall y José
