#include <cstdint>
#include <stdexcept>
#include "messagesTypes.hpp"

constexpr std::uint64_t align_to(std::uint64_t x, std::uint64_t a) {
    return (x + (a - 1)) & ~(a - 1);
}

// Garantiza que la memoria es múltiplo del tamaño de línea
static_assert(MEM_BYTES % LINE_SIZE == 0, "MEM_BYTES debe ser múltiplo de LINE_SIZE");

struct MemLayout {
    // parámetros
    std::uint32_t N = 0;
    std::uint32_t NUM_HW_THREADS = 0;
    std::uint32_t NUM_SYSTEM_PES = 0;

    // bases (alineadas a LINE_SIZE)
    std::uint64_t BASE_CONSTS   = 0x0000;
    std::uint64_t BASE_A        = 0;
    std::uint64_t BASE_B        = 0;
    std::uint64_t BASE_PARTIALS = 0;
    std::uint64_t BASE_RESULT   = 0;

    // tamaños
    std::uint64_t SIZE_CONSTS   = 256;
    std::uint64_t SIZE_A        = 0;   
    std::uint64_t SIZE_B        = 0;   
    std::uint64_t SIZE_PARTIALS = 0;   
    std::uint64_t SIZE_RESULT   = 8;

    std::uint64_t END_CONSTS    = 0;
    std::uint64_t END_A         = 0;
    std::uint64_t END_B         = 0;
    std::uint64_t END_PARTIALS  = 0;
    std::uint64_t END_RESULT    = 0;

    // Calcula el layout usando LINE_SIZE/MEM_BYTES del types
    // align_partials_to_line=true reserva 1 línea por PE para evitar false sharing
    void compute(std::uint32_t n,
                 std::uint32_t hw_threads,
                 std::uint32_t pes,
                 bool align_partials_to_line)
    {
        N = n; NUM_HW_THREADS = hw_threads; NUM_SYSTEM_PES = pes;

        SIZE_A = std::uint64_t(N) * 8;
        SIZE_B = std::uint64_t(N) * 8;
        SIZE_PARTIALS = align_partials_to_line
                      ? std::uint64_t(pes) * LINE_SIZE
                      : std::uint64_t(pes) * 8;

        BASE_CONSTS   = align_to(BASE_CONSTS, LINE_SIZE);
        END_CONSTS    = BASE_CONSTS + SIZE_CONSTS;

        BASE_A        = align_to(END_CONSTS, LINE_SIZE);
        END_A         = BASE_A + SIZE_A;

        BASE_B        = align_to(END_A, LINE_SIZE);
        END_B         = BASE_B + SIZE_B;

        BASE_PARTIALS = align_to(END_B, LINE_SIZE);
        END_PARTIALS  = BASE_PARTIALS + SIZE_PARTIALS;

        BASE_RESULT   = align_to(END_PARTIALS, LINE_SIZE);
        END_RESULT    = BASE_RESULT + SIZE_RESULT;

        // Verificación de capacidad contra MEM_BYTES (del types)
        if (END_RESULT > MEM_BYTES) {
            throw std::runtime_error("MemLayout: no cabe en MEM_BYTES con la configuración dada.");
        }
    }
};
