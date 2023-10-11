#include <gtest/gtest.h>
#include <cstdint>

#include "68000.h"

constexpr auto SignBit(auto v) -> bool {
    const auto msb = sizeof(v) * CHAR_BIT - 1u;
    const auto mask = 1u << msb;
    return v & mask;
}

constexpr auto SameSignBit(uint32_t dividend, uint16_t divisor) -> bool {
    return !(SignBit(dividend) ^ SignBit(divisor));
}

constexpr auto AbsoluteValue(auto v) -> decltype(v) {
    return SignBit(v) ? -v : v;
}

struct DivsResult {
    uint16_t remainder;
    uint16_t quotient;
};

constexpr auto DivideSigned(uint32_t dividend, uint16_t divisor) -> DivsResult {
    const DivsResult failure = {
        static_cast<uint16_t>(dividend >> 16u),
        static_cast<uint16_t>(dividend)
    };
    if (divisor == 0u) {
        return failure;
    }
    const auto absDividend = AbsoluteValue(dividend);
    const auto absDivisor = AbsoluteValue(divisor);
    const auto absQuotient = absDividend / absDivisor;
    const auto absRemainder = absDividend % absDivisor;
    if (absQuotient >= 0x1'0000u) {
        return failure;
    }
    // remainder same sign as dividend
    const uint16_t remainder = SignBit(dividend) ? -absRemainder : absRemainder;
    const uint16_t quotient = SameSignBit(dividend, divisor) ? absQuotient : -absQuotient;
    if (SameSignBit(dividend, divisor)) {
        // quotient should be greater than equal to zero
        if (SignBit(quotient)) {
            return failure;
        }
    } else if (!SignBit(quotient) && (quotient != 0u)) {
        // quotient should be less than or equal to zero
        return failure;
    }
    return { remainder, quotient };
}

constexpr auto DivideSignedCycles(uint32_t dividend, uint16_t divisor) -> uint32_t {
    auto cycles = 2u * 2u; // DVS01, DVS03

    if (divisor == 0u) {
        // Note: we should be including the exception timing here
        return 0u;
    }

    if (SignBit(dividend)) {
        cycles += 5u * 2u; // DVS04/5, DVS06, DVS10, DVS11, DVS08
    } else {
        cycles += 4u * 2u; // DVS04/5, DVS06, DVS07, DVS08,
    }

    auto absDividend = AbsoluteValue(dividend);
    auto absDivisor = AbsoluteValue(divisor);

    if (absDividend / absDivisor >= 0x1'0000u) {
        cycles += 2u * 2u; // DVUMZ, DVUMA
        return cycles;
    }

    const auto alignedDivisor = absDivisor << 16u;
    for (auto i = 0u; i < 15u; ++i) {
        cycles += 3u * 2u; // DVS09/A DVS0C, DVS0D
        absDividend <<= 1u;
        if (absDividend >= alignedDivisor) {
            absDividend -= alignedDivisor;
        } else {
            cycles += 2u; // DVS0F
        }
    }
    cycles += 5u * 2u; // DVS09/A, DVS0C, DVS0E, DVS12/13, DVS14

    if (SignBit(divisor)) {
        // DVS15, DVS1D, DVS1F, DVS20
        // DVS15, DVS1D, DVS1E, DVS1C
        // DVS15, DVS1D, DVS1E, DVUM4
        cycles += 4u * 2u;
    } else if (SignBit(dividend)) {
        // DVS15, DVS16, DVS1A, DVS1B, DVS1C
        // DVS15, DVS16, DVS1A, DVS1B, DVUM4
        cycles += 5u * 2u;
    } else {
        // DVS15, DVS16, DVS17
        cycles += 3u * 2u;
    }
    cycles += 1u * 2u; // LEAA2 or DVUMA

    return cycles;
}

struct DivsTestParam {
    uint32_t dividend;
    uint16_t divisor;
};

struct DivsTestFixture : public testing::TestWithParam<DivsTestParam> {
    MC68000 mc68000;
};

TEST_P(DivsTestFixture, TestSignedDivision) {
    const auto&[dividend, divisor] = GetParam();
    const auto&[remainder, quotient] = DivideSigned(dividend, divisor);
    mc68000.rxdh = dividend >> 16u;
    mc68000.rxdl = dividend;
    mc68000.rydl = divisor;
    mc68000.ExecuteDivs();
    EXPECT_EQ(mc68000.rxdh, remainder);
    EXPECT_EQ(mc68000.rxdl, quotient);
    EXPECT_EQ(mc68000.rydl, divisor);
    EXPECT_EQ(mc68000.cycles, DivideSignedCycles(dividend, divisor));
}

constexpr DivsTestParam DIVS_TEST_PARAMETERS[] = {
    // Basic tests
    { 29u,            5u }, // Test positive dividend and divisor
    { 29u,            static_cast<uint16_t>(-5u) }, // Test positive dividend and negative divisor
    { -29u,           5u }, // Test negative dividend and positive divisor
    { -29u,           static_cast<uint16_t>(-5u) }, // Test negative dividend and divisor
    { 0u,             5u },
    { 0u,             static_cast<uint16_t>(-5u) },
    // Early overflow
    { 0x5A5A'0000u,   0x5959u }, // +ve dividend
    { 0x8003'0000u,   0x0001u }, // -ve dividend
    // Late overflow tests
    { 0x0000'8000,    1u }, // +ve / +ve
    { 0xffff'0001,    1u }, // -ve / +ve
    { 0x4000'8000,    0x8000u }, // +ve / -ve
    { 0x8000'0001,    0x8000u }, // -ve / -ve
    // Misc timing
    { 0x5A'5A'00'08u, 0x5A5Bu },
    { 0x8000u,        1u },
};

INSTANTIATE_TEST_SUITE_P(DivsTest, DivsTestFixture, ::testing::ValuesIn(DIVS_TEST_PARAMETERS));