/*
  pe.cpp — PE con “ISA mínima” y 8 registros de 64 bits.
  Instrucciones (documental): LOAD64, STORE64, ADD, MUL, INC, DEC, MOV
  Demostración: si exportas PE_TRACE=1 imprime una traza por-PE
  con el conteo de instrucciones ejecutadas y el resultado parcial.
*/
#include "pe/pe.hpp"
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <iostream>

static inline double   unpack_double(uint64_t u){ double d; std::memcpy(&d,&u,sizeof(double)); return d; }
static inline uint64_t pack_double(double  x){ uint64_t u; std::memcpy(&u,&x,sizeof(double)); return u; }

// ISA mínima (enumeración documental; no se usa para decodificar)
enum class Op { LOAD64, STORE64, ADD, MUL, INC, DEC, MOV };

void PE::setup(int pe_id, IDataMem* m, const RunConfig& c, PeMetrics* slot) {
  id = pe_id; mem = m; cfg = c; mx = slot;
}

void PE::run_kernel() {
  // División en 4 PEs; el último toma el residuo
  int base = (cfg.N / 4) * id;
  int rest = cfg.N % 4;
  int extra = (id == 3 ? rest : 0);
  int nloc = (cfg.N / 4) + extra;

  // 8 registros de 64-bit
  // R0: ptrA  | R1: ptrB | R2: count | R3: valA | R4: valB
  // R5: acc   | R6: ptrPartial       | R7: temp/scratch
  uint64_t R[8]{}; 

  // Traza opcional
  bool trace = false;
  if (const char* t = std::getenv("PE_TRACE")) trace = (t[0] == '1');

  // Contadores locales por instrucción (solo para la traza)
  uint64_t c_mov=0, c_inc=0, c_dec=0, c_load=0, c_store=0, c_add=0, c_mul=0;

  // Helpers tipo “instrucción”
  auto MOV  = [&](int rd, uint64_t imm) { R[rd] = imm; c_mov++; };
  auto INC  = [&](int rd) { R[rd] += 8ull; c_inc++; };            // +8 B (double)
  auto DEC  = [&](int rd) { if (R[rd] > 0) R[rd] -= 1ull; c_dec++; };

  auto LOAD64 = [&](int rd, int raddr) {
    uint64_t u = mem->load64(R[raddr]);
    R[rd] = u;
    c_load++;
    if (mx) mx->loads += 1;
  };
  auto STORE64 = [&](int raddr, int rs) {
    mem->store64(R[raddr], R[rs]);
    c_store++;
    if (mx) mx->stores += 1;
  };
  auto MUL = [&](int rd, int rs) {
    double a = unpack_double(R[rd]);
    double b = unpack_double(R[rs]);
    R[rd] = pack_double(a * b);
    c_mul++;
  };
  auto ADD = [&](int rd, int rs) {
    double a = unpack_double(R[rd]);
    double b = unpack_double(R[rs]);
    R[rd] = pack_double(a + b);
    c_add++;
  };

  const uint64_t stride = cfg.align32 ? 32ull : 8ull;

  // Programa: inicialización
  MOV(0, cfg.baseA + (uint64_t)base * 8ull);           // ptrA
  MOV(1, cfg.baseB + (uint64_t)base * 8ull);           // ptrB
  MOV(2, (uint64_t)nloc);                              // count
  MOV(5, pack_double(0.0));                            // acc = 0.0
  MOV(6, cfg.basePartial + (uint64_t)id * stride);     // ptrPartial

  // Bucle: mientras R2 (count) != 0
  while (R[2] != 0ull) {
    LOAD64(/*rd=*/3, /*[Raddr]=*/0);   // R3 = *R0 (A)
    LOAD64(/*rd=*/4, /*[Raddr]=*/1);   // R4 = *R1 (B)
    MUL(/*rd=*/3, /*rs=*/4);           // R3 = R3 * R4
    ADD(/*rd=*/5, /*rs=*/3);           // R5 = R5 + R3
    INC(0);                            // R0 += 8
    INC(1);                            // R1 += 8
    DEC(2);                            // R2 -= 1
  }

  // Escribir parcial
  STORE64(/*[Raddr]=*/6, /*rs=*/5);
  if (mx) mx->result = unpack_double(R[5]);

  if (trace) {
    uint64_t total = c_mov + c_inc + c_dec + c_load + c_store + c_add + c_mul;
    std::cerr << "[PE" << id << "][trace] ISA=MIN7 regs=8  "
              << "MOV="   << c_mov
              << " LOAD=" << c_load
              << " STORE="<< c_store
              << " ADD="  << c_add
              << " MUL="  << c_mul
              << " INC="  << c_inc
              << " DEC="  << c_dec
              << " TOTAL="<< total
              << "  result=" << unpack_double(R[5])
              << "\n";
  }
}
