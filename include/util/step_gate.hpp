#pragma once
#include <atomic>
#include <iostream>
#include <string>

struct StepGate {
  // flags activados desde main (por env var MESI_STEP)
  static inline std::atomic<bool> on_bus{false};
  static inline std::atomic<bool> on_mem{false};

  static inline void set(bool bus, bool mem) {
    on_bus.store(bus);
    on_mem.store(mem);
  }

  static inline void wait_if_bus(const std::string& msg) {
    if (!on_bus.load()) return;
    std::cerr << "[STEP][BUS] " << msg << "  (ENTER para continuar)" << std::endl;
    std::string _; std::getline(std::cin, _);
  }

  static inline void wait_if_mem(const std::string& msg) {
    if (!on_mem.load()) return;
    std::cerr << "[STEP][MEM] " << msg << "  (ENTER para continuar)" << std::endl;
    std::string _; std::getline(std::cin, _);
  }
};
