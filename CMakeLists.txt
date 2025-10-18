# -----------------------------------------------------------------------------
# Proyecto: mp-mesi-sim
# Configuración de build (C++17) y módulos del simulador.
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(mp_mesi_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MP_USE_MOCKS "Compilar usando implementaciones mock (sin L1/Bus/Mem reales)" ON)

# Notas de include:
# - Algunos headers de l1/busmem usan includes del tipo "include/...".
#   Por eso añadimos ${CMAKE_SOURCE_DIR} además de ${CMAKE_SOURCE_DIR}/include.
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Tu módulo (PE/Loader/Metrics)
file(GLOB PE_SRC src/pe/*.cpp)
add_library(pe_objs OBJECT ${PE_SRC})

# Mocks (permite correr sin L1/Bus)
file(GLOB MOCK_SRC src/mock/*.cpp)
add_library(mp_mock ${MOCK_SRC})

# L1 y Bus/Mem (de Randall/José)
file(GLOB L1_SRC     src/l1/*.cpp)
file(GLOB BUSMEM_SRC src/busmem/*.cpp)
add_library(l1_objs     OBJECT ${L1_SRC})
add_library(busmem_objs OBJECT ${BUSMEM_SRC})

# Ejecutable principal
add_executable(sim src/app/main.cpp)

# Enlace: siempre linkeamos pe_objs y además l1_objs/busmem_objs.
# mp_mock queda para el modo "mock" del runner.
target_link_libraries(sim PRIVATE pe_objs l1_objs busmem_objs mp_mock)

enable_testing()

# --- test mínimo para L1 bloqueante ---
add_executable(l1_blocking_smoke tests/l1_blocking_smoke.cpp)
target_link_libraries(l1_blocking_smoke PRIVATE l1_objs busmem_objs)
