#include <gtest/gtest.h>
#include <cstdint>

#include "68000.h"

struct DivuResult {
    uint16_t remainder;
    uint16_t quotient;
};

constexpr auto DivideUnsigned(uint32_t dividend, uint16_t divisor) -> DivuResult {
    const DivuResult failure = {
        static_cast<uint16_t>(dividend >> 16u),
        static_cast<uint16_t>(dividend)
    };
    if (divisor == 0u) {
        return failure;
    }
    const auto quotient = dividend / divisor;
    const auto remainder = dividend % divisor;
    if (quotient >= 0x1'0000u) {
        return failure;
    }
    return { static_cast<uint16_t>(remainder), static_cast<uint16_t>(quotient) };
}

constexpr auto DivideUnsignedCycles(uint32_t dividend, uint16_t divisor) -> uint32_t {
    auto cycles = 2u * 2u; // DVUR1, DVUM2

    if (divisor == 0u) {
        // Note: we should be including the exception timing here
        return 0u;
    }

    cycles += 1u * 2u; // DVUM3

    if (dividend / divisor >= 0x1'0000u) {
        cycles += 2u * 2u; // DVUM4, DVUMA
        return cycles;
    }

    const auto alignedDivisor = divisor << 16u;
    for (auto i = 0u; i < 15u; ++i) {
        cycles += 2u * 2u; // DVUM5/6 DVUM7/8
        const auto previous = dividend;
        dividend <<= 1u;
        if (previous & 0x8000'0000u) {
            dividend -= alignedDivisor;
        } else if (dividend >= alignedDivisor) {
            cycles += 1u * 2u; // DVUMB
            dividend -= alignedDivisor;
        } else {
            cycles += 2u * 2u; // DVUMB DVUME
        }
    }

    cycles += 4u * 2u; // DVUM5/6 DVUM7/8 DVUM9/C DVUMD/F
    cycles += 1u * 2u; // DVUM0

    return cycles;
}

struct DivuTestParam {
    uint32_t dividend;
    uint16_t divisor;
};

struct DivuTestFixture : public testing::TestWithParam<DivuTestParam> {
    MC68000 mc68000;
};

TEST_P(DivuTestFixture, TestSignedDivision) {
    const auto&[dividend, divisor] = GetParam();
    const auto&[remainder, quotient] = DivideUnsigned(dividend, divisor);
    mc68000.rxdh = dividend >> 16u;
    mc68000.rxdl = dividend;
    mc68000.rydl = divisor;
    mc68000.ExecuteDivu();
    EXPECT_EQ(mc68000.rxdh, remainder);
    EXPECT_EQ(mc68000.rxdl, quotient);
    EXPECT_EQ(mc68000.rydl, divisor);
    EXPECT_EQ(mc68000.cycles, DivideUnsignedCycles(dividend, divisor));
}

constexpr DivuTestParam DIVU_TEST_PARAMETERS[] = {
    // Basic values
    { 29u,            5u },
    { 5u,             29u },
    { 392u,           17u },
    { 9911u,          605u },
    { 0u,             1u },
    { 0u,             317u },
    // Misc timing
    { 0x04'32'10'FFu, 0x5A5Bu },
    { 0x5A'5A'00'08u, 0x5A5Bu },
    { 0xA5'A5'CC'DDu, 0xA6A6u },
    { 0xF5'AF'CC'DDu, 0xF6A6u },
    // Overflow tests
    { 0x00'02'00'00u, 0x0001u },
    { 0x5A'5A'00'00u, 0x0001u },
    { 0x5A'5A'00'00u, 0x5A5Au },
};

INSTANTIATE_TEST_SUITE_P(DivuTest, DivuTestFixture, ::testing::ValuesIn(DIVU_TEST_PARAMETERS));