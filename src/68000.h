#pragma once

#include <cstdint>

// DIVU microword labels

constexpr auto DVUR1 = 1u;
constexpr auto DVUM2 = 2u;
constexpr auto DVUM3 = 3u;
constexpr auto DVUM4 = 4u;
constexpr auto DVUM5 = 5u;
constexpr auto DVUM6 = 6u;
constexpr auto DVUM7 = 7u;
constexpr auto DVUM8 = 8u;
constexpr auto DVUM9 = 9u;
constexpr auto DVUMA = 10u;
constexpr auto DVUMB = 11u;
constexpr auto DVUMC = 12u;
constexpr auto DVUMD = 13u;
constexpr auto DVUME = 14u;
constexpr auto DVUMF = 15u;
constexpr auto DVUM0 = 16u;
constexpr auto DVUMZ = 20u;


// DIVS microword labels
constexpr auto DVS01 = 101u; // Note the patent says signed division stars with DVS02 but that reads from the data bus
constexpr auto DVS03 = 103u;
constexpr auto DVS04 = 104u;
constexpr auto DVS05 = 105u;
constexpr auto DVS06 = 106u;
constexpr auto DVS07 = 107u;
constexpr auto DVS08 = 108u;
constexpr auto DVS09 = 109u;
constexpr auto DVS0A = 110u;
constexpr auto DVS0C = 112u;
constexpr auto DVS0D = 113u;
constexpr auto DVS0E = 114u;
constexpr auto DVS0F = 115u;
constexpr auto DVS10 = 116u;
constexpr auto DVS11 = 117u;
constexpr auto DVS12 = 118u;
constexpr auto DVS13 = 119u;
constexpr auto DVS14 = 120u;
constexpr auto DVS15 = 121u;
constexpr auto DVS16 = 122u;
constexpr auto DVS17 = 123u;
constexpr auto DVS1A = 126u;
constexpr auto DVS1B = 127u;
constexpr auto DVS1C = 128u;
constexpr auto DVS1D = 129u;
constexpr auto DVS1E = 130u;
constexpr auto DVS1F = 131u;
constexpr auto DVS20 = 132u;

// LEA microword
constexpr auto LEAA2 = 200u;

// TRAP microword labels
constexpr auto TRAP0 = 300u;

// A1, A2, A3 instruction microword labels
constexpr auto A1 = 400u;

// Processor flags
constexpr auto FLAG_X = 0x10u;
constexpr auto FLAG_N = 0x08u;
constexpr auto FLAG_Z = 0x04u;
constexpr auto FLAG_V = 0x02u;
constexpr auto FLAG_C = 0x01u;

struct MC68000 {
    uint16_t microword{};

    uint16_t rxdh{}; // Data register x
    uint16_t rxdl{};
    uint16_t rydl{}; // Data register y

    uint32_t pc{}; // Program counter

    uint16_t alue{}; // Alu extender
    uint16_t alub{}; // Alu buffer

    uint16_t alu{}; // Alu
    uint16_t flags{}; // Flags

    uint32_t au{}; // Arithmetic unit

    uint16_t ath{}; // Address temporary register
    uint16_t atl{};

    uint32_t cycles {};

    auto AluOp_AND(uint16_t, uint16_t) -> uint16_t;
    auto AluOp_SUB(uint16_t, uint16_t) -> uint16_t;
    auto AluOp_SUBX(uint16_t, uint16_t) -> uint16_t;
    auto AluOp_SLAAx(uint16_t) -> uint16_t;

    auto Print() const -> void;

    auto ExecuteDivu() -> void;
    auto ExecuteDivs() -> void;

};