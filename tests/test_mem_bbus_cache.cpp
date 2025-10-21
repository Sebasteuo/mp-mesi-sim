#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <memory>

#include "include/busmem/memory.hpp"
#include "include/busmem/bus.hpp"
#include "include/l1/cache.hpp"

// Función para simular el trabajo de un Procesador (PE)
void pe_thread_func(int id, Cache& l1_cache, uint64_t addr) {
  std::cout << "\n--- PE" << id << " inicia su trabajo ---\n";

  if (id == 0) {
    // 1. PE0 lee
    std::cout << "[PE0] Leyendo de la dirección " << addr << " (debería ser un Read Miss -> E)"
              << std::endl;
    l1_cache.read<double>(addr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Dar tiempo al bus para procesar

    // 3. PE0 escribe
    double value_to_write = 3.1416;
    std::cout << "[PE0] Escribiendo " << value_to_write << " en la dirección " << addr
              << " (debería ser un Upgrade Miss -> M)" << std::endl;
    l1_cache.write<double>(addr, value_to_write);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } else if (id == 1) {
    // 2. PE1 lee
    std::cout << "[PE1] Leyendo de la dirección " << addr << " (debería ser un Read Miss -> S)"
              << std::endl;
    l1_cache.read<double>(addr);
    std::this_thread::sleep_for(
        std::chrono::milliseconds(200)); // Esperar más tiempo para el próximo paso
  } else if (id == 2) {
    // 4. PE2 lee
    std::cout << "[PE2] Leyendo de la dirección " << addr
              << " (debería ser un Read Miss, servido por L1-0 -> S)" << std::endl;
    l1_cache.read<double>(addr);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::cout << "--- PE" << id << " terminó su trabajo ---\n";
}

int main() {
  std::cout << "--- Configuración del Simulador ---" << std::endl;

  // 1. Crear la Memoria
  SharedMemory memory;

  // 2. Crear las Cachés (aún no conocen al bus)
  std::vector<std::unique_ptr<Cache>> caches;
  std::array<ICache*, L1_COUNT> cache_pointers;
  for (int i = 0; i < L1_COUNT; ++i) {
    caches.push_back(std::make_unique<Cache>(i));
    cache_pointers[i] = caches[i].get();
  }
  std::cout << "Cachés creadas.\n";

  // 3. Crear el Bus, ahora que tenemos los punteros a las cachés
  Bus bus(memory, cache_pointers);
  std::cout << "Bus creado.\n";

  // 4. Conectar las cachés al bus
  for (auto& cache : caches) {
    cache->setBus(bus);
  }
  std::cout << "Cachés conectadas al bus.\n";

  // Iniciar el hilo del bus
  std::thread bus_thread(&Bus::run, &bus);
  // bus_thread.detach();

  std::cout << "\n--- Inicio de la Simulación de Carga de Trabajo ---\n";

  const uint64_t test_addr = 100;

  // Simular PEs como hilos separados
  std::thread pe0(pe_thread_func, 0, std::ref(*caches[0]), test_addr);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  std::thread pe1(pe_thread_func, 1, std::ref(*caches[1]), test_addr);
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  std::thread pe2(pe_thread_func, 2, std::ref(*caches[2]), test_addr);

  pe0.join();
  pe1.join();
  pe2.join();

  bus.stop();
  bus_thread.join();

  std::cout << "\n--- Simulación Terminada ---" << std::endl;

  return 0;
}