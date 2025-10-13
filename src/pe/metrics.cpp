/*
  Archivo: metrics.cpp
  Implementación de helpers para preparar y exportar métricas a CSV.
*/
#include "util/metrics.hpp"
#include <fstream>
#include <iomanip>

void Metrics::resize(int pes) {
  per_pe.clear();
  per_pe.resize(pes);
  for (int i = 0; i < pes; ++i) per_pe[i].id = i;
}

void Metrics::to_csv(const std::string& path) const {
  std::ofstream f(path);
  f << "run_id,config_str,pe_id,"
       "hits,misses_r,misses_w,inval_sent,inval_recv,"
       "loads,stores,"
       "bus_msgs_ctrl,bus_bytes_ctrl,bus_lines_data,bus_bytes_data,"
       "ticks,result,abs_error\n";
  for (const auto& p : per_pe) {
    f << run_id << ","
      << config_str << ","
      << p.id << ","
      << p.hits << "," << p.misses_r << "," << p.misses_w << ","
      << p.inval_sent << "," << p.inval_recv << ","
      << p.loads << "," << p.stores << ","
      << p.bus_msgs_ctrl << "," << p.bus_bytes_ctrl << ","
      << p.bus_lines_data << "," << p.bus_bytes_data << ","
      << p.ticks << ","
      << std::setprecision(17) << p.result << ","
      << std::scientific << p.abs_error << "\n";
  }
}
