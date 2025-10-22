#ifndef MESSAGES_TYPES_HPP
#define MESSAGES_TYPES_HPP

// Tamaños/latencias
constexpr int LINE_SIZE = 32;   // bytes
constexpr int MEM_BYTES = 4096; // 512 x 8 B
constexpr int L1_HIT_LAT = 1;
constexpr int BUS_LAT = 10;
constexpr int MEM_LAT = 40;

constexpr int L1_CACHE_WAYS = 2;
constexpr int L1_CACHE_SETS = 16;

// Identificadores de origen (L1 de cada PE)
enum : int { L1_0 = 0, L1_1 = 1, L1_2 = 2, L1_3 = 3, L1_COUNT = 4 };

// Tipos de mensaje en el bus
enum class BusMsgType { BusRd, BusUp, Invalidate, Flush, Data };

#endif // MESSAGES_TYPES_HPP