/*
  loader.cpp
  Prepara A, B y limpia partial_sums.
  Si --align32 está activo, los parciales se separan 32 bytes (1 línea de 32B).
*/
#include "pe/loader.hpp"
#include <cstring>     
#include "include/busmem/messagesTypes.hpp"

static inline uint64_t pack_double(double x) {
  uint64_t u; std::memcpy(&u, &x, sizeof(double)); return u;
}
static inline void store64(IDataMem* m, std::uint64_t addr, std::uint64_t u64) {
    m->store64(addr, u64);
}
static inline void storeU32(IDataMem* m, std::uint64_t addr, std::uint32_t u32) {
    m->store64(addr, static_cast<std::uint64_t>(u32));
}

void Loader::init(IDataMem* m, const RunConfig& c) {
    mem = m;
    cfg = c;

    // 1) Definir layout (ajusta HW threads / #PEs si los tienes en RunConfig)
    const std::uint32_t HW_THREADS = 4;    // <- si tienes cfg.hw_threads, úsalo aquí
    const std::uint32_t NUM_PES    = 4;    // <- si tienes cfg.num_pes, úsalo aquí
    setup_layout(/*N=*/cfg.N, HW_THREADS, NUM_PES, /*align32=*/cfg.align32);

    // 2) Escribir CONSTANTES visibles para el ASM/PEs
    write_consts();

    // 3) Inicializaciones de contenido (vectores A/B, parciales y resultado)
    load_vectors();
    clear_partials();
    store64(mem, layout.BASE_RESULT, pack_double(0.0));
}

void Loader::setup_layout(std::uint32_t N,
                          std::uint32_t hw_threads,
                          std::uint32_t pes,
                          bool align32)
{
    // Calcula el mapa de memoria (segmentación)
    layout.compute(N, hw_threads, pes, /*align_partials_to_line=*/align32);

    // Mantener compatibilidad con el resto del código: propaga bases a cfg
    cfg.baseA       = layout.BASE_A;
    cfg.baseB       = layout.BASE_B;
    cfg.basePartial = layout.BASE_PARTIALS;

    // Si no existe en tu RunConfig, puedes omitir esta asignación
    // o añadir el campo baseResult al RunConfig.
    // cfg.baseResult  = layout.BASE_RESULT;
}

void Loader::write_consts() {
    const std::uint64_t C = layout.BASE_CONSTS;

    // Offsets (múltiplos de 8 para que calcen en línea):
    // 0x00: N (u32)          0x08: NUM_HW_THREADS (u32)   0x10: NUM_SYSTEM_PES (u32)
    // 0x18: BASE_A (u64)     0x20: BASE_B (u64)           0x28: BASE_PARTIALS (u64)
    // 0x30: BASE_RESULT (u64)
    storeU32(mem, C + 0x00, layout.N);
    storeU32(mem, C + 0x08, layout.NUM_HW_THREADS);
    storeU32(mem, C + 0x10, layout.NUM_SYSTEM_PES);

    store64(mem, C + 0x18, layout.BASE_A);
    store64(mem, C + 0x20, layout.BASE_B);
    store64(mem, C + 0x28, layout.BASE_PARTIALS);
    store64(mem, C + 0x30, layout.BASE_RESULT);
}

void Loader::load_vectors() {
    // Escribe A y B en las bases calculadas por el layout.
    // Ajusta aquí si en tu RunConfig tienes otras fuentes de datos.
    for (std::uint32_t i = 0; i < cfg.N; ++i) {
        const double a = 1.0 + static_cast<double>(i);
        const double b = 0.5 * static_cast<double>(i) - 1.0;
        store64(mem, cfg.baseA + static_cast<std::uint64_t>(i)*8ull, pack_double(a));
        store64(mem, cfg.baseB + static_cast<std::uint64_t>(i)*8ull, pack_double(b));
    }
}

void Loader::clear_partials() {
    // Si align32=true, reservamos una LÍNEA por PE (evita false sharing):
    const std::uint64_t stride = cfg.align32 ? static_cast<std::uint64_t>(LINE_SIZE) : 8ull;

    for (std::uint32_t pe = 0; pe < layout.NUM_SYSTEM_PES; ++pe) {
        const std::uint64_t addr = cfg.basePartial + pe * stride;
        store64(mem, addr, pack_double(0.0));
    }
}