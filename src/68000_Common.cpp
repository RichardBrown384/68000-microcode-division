#include <iostream>
#include <bitset>

#include "68000.h"

auto MC68000::AluOp_AND(uint16_t dst, uint16_t src) -> uint16_t {
    const auto oldFlags = flags;
    alu = dst & src;
    flags &= FLAG_X;
    flags |= (alu & 0x8000u) ? FLAG_N : 0u;
    flags |= (alu == 0u) ? FLAG_Z : 0u;
    return oldFlags;
}

auto MC68000::AluOp_SUB(uint16_t dst, uint16_t src) -> uint16_t {
    const auto oldFlags = flags;
    alu = dst - src;
    const auto overflow = (dst ^ src) & (dst ^ alu);
    const auto carry = (dst ^ src) ^ alu ^ overflow;
    flags = 0u;
    flags |= (carry & 0x8000u) ? FLAG_X : 0u;
    flags |= (alu & 0x8000u) ? FLAG_N : 0u;
    flags |= (alu == 0u) ? FLAG_Z : 0u;
    flags |= (overflow & 0x8000u) ? FLAG_V : 0u;
    flags |= (carry & 0x8000u) ? FLAG_C : 0u;
    return oldFlags;
}

auto MC68000::AluOp_SUBX(uint16_t dst, uint16_t src) -> uint16_t {
    alu = dst - src - ((flags & FLAG_X) >> 4u);
    return flags;
}

auto MC68000::AluOp_SLAAx(uint16_t leastSignificantBit) -> uint16_t {
    alu <<= 1u;
    alu += (alue >> 15u) & 1u;
    alue <<= 1u;
    alue += leastSignificantBit;
    return flags;
}

auto MC68000::Print() const -> void {
    std::cout << "Microword: " << std::to_string(microword) << std::endl;
    std::cout << "Loop counter (au): " << std::to_string(au) << std::endl;
    std::cout << "Alu : " << std::bitset<16>(alu) << " Alue: " << std::bitset<16>(alue) << std::endl;
    std::cout << "Alub: " << std::bitset<16>(alub) << std::endl;
    std::cout << "Rxdh : " << std::to_string(rxdh) << " Rxdl: " << std::to_string(rxdl) << std::endl;
    std::cout << "Rydl : " << std::to_string(rydl) << std::endl;
}