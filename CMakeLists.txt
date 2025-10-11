cmake_minimum_required(VERSION 3.20)
project(mp_mesi_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MP_USE_MOCKS "Usar implementaciones mock para correr sin L1/Bus reales" ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Librerías por módulo (se llenarán luego)
add_library(l1_objs    OBJECT)  # Randall (L1)
add_library(busmem_objs OBJECT) # José (Bus+Mem)
add_library(pe_objs     OBJECT) # Sebastián (PE/Loader/Metrics)

# Mocks (para que Sebastián corra desde ya)
file(GLOB MOCK_SRC src/mock/*.cpp)
add_library(mp_mock ${MOCK_SRC})

# Ejecutable principal
add_executable(sim src/app/main.cpp)
if(MP_USE_MOCKS)
  target_link_libraries(sim PRIVATE mp_mock)
else()
  # Cuando estén listas: linkear con las libs reales
  target_link_libraries(sim PRIVATE l1_objs busmem_objs pe_objs)
endif()

enable_testing()
