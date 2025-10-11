cmake_minimum_required(VERSION 3.20)
project(mp_mesi_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MP_USE_MOCKS "Usar implementaciones mock para correr sin L1/Bus reales" ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Mocks (para que Sebastián pruebe sin esperar a L1/Bus/Mem)
file(GLOB MOCK_SRC src/mock/*.cpp)
add_library(mp_mock ${MOCK_SRC})

# Ejecutable principal
add_executable(sim src/app/main.cpp)

if(MP_USE_MOCKS)
  # Con mocks no necesitamos compilar módulos vacíos
  target_link_libraries(sim PRIVATE mp_mock)
else()
  # Cuando Randall y José agreguen código real:
  file(GLOB L1_SRC     src/l1/*.cpp)
  file(GLOB BUSMEM_SRC src/busmem/*.cpp)
  file(GLOB PE_SRC     src/pe/*.cpp)

  add_library(l1_objs     OBJECT ${L1_SRC})
  add_library(busmem_objs OBJECT ${BUSMEM_SRC})
  add_library(pe_objs     OBJECT ${PE_SRC})

  target_link_libraries(sim PRIVATE l1_objs busmem_objs pe_objs)
endif()

enable_testing()
