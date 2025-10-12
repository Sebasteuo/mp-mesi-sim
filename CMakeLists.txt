# -----------------------------------------------------------------------------
# Proyecto: mp-mesi-sim
# Descripción:
#   Configura la compilación del simulador en C++17.
#   - Se puede compilar con implementaciones "mock" para poder ejecutar el
#     programa aunque las partes de L1 y Bus/Mem todavía no estén listas.
#   - Cuando L1 y Bus/Mem estén implementados, se pueden enlazar en lugar
#     de los mocks usando la misma estructura.
#
# Cómo se usa:
#   mkdir -p build && cd build
#   cmake .. -DMP_USE_MOCKS=ON -DCMAKE_BUILD_TYPE=Release
#   cmake --build . -- -j 4
#
# Notas:
#   - No se usan esperas reales (sleep). El proyecto trabaja con tiempo lógico.
#   - Se separan fuentes por módulos para que el trabajo sea paralelo.
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)
project(mp_mesi_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Bandera para elegir si se compila con mocks (útil mientras L1/Bus/Mem se implementan).
option(MP_USE_MOCKS "Compilar usando implementaciones mock (sin L1/Bus/Mem reales)" ON)

# Directorios de cabeceras
include_directories(${CMAKE_SOURCE_DIR}/include)

# PE/Loader/Metrics:
#   Se compilan siempre porque permiten ejecutar el kernel y generar CSV.
file(GLOB PE_SRC src/pe/*.cpp)
add_library(pe_objs OBJECT ${PE_SRC})

# Mocks:
#   Implementaciones mínimas para poder correr el binario sin dependencias externas.
file(GLOB MOCK_SRC src/mock/*.cpp)
add_library(mp_mock ${MOCK_SRC})

# Ejecutable principal (punto de entrada)
add_executable(sim src/app/main.cpp)

# Enlace:
#   Con mocks activos se enlaza pe_objs + mp_mock.
#   Más adelante se podrán enlazar las bibliotecas de L1 y Bus/Mem reales.
if(MP_USE_MOCKS)
  target_link_libraries(sim PRIVATE pe_objs mp_mock)
else()
  target_link_libraries(sim PRIVATE pe_objs mp_mock)
endif()

enable_testing()
