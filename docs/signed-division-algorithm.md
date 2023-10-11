# 68000 signed division

## Table of Contents

1. [Introduction](#introduction)
2. [Outline of signed division in hardware](#outline-of-signed-division-in-hardware)
3. [Signed division microcode](#signed-division-microcode)
4. [Conclusion](#conclusion)

## Introduction

The purpose of this document is to attempt to explain the workings of the 68000 signed division algorithm and
infer the correct timing properties.

The 68000 supports dividing 32-bit signed integers by 16-bit signed integers.

The result is a 32-bit integer with the quotient in the lower 16-bits and the remainder in the upper 16-bits.

The [unsigned division](./unsigned-division-algorithm.md) document is prerequisite reading. Signed division
uses the same concepts and unsigned division and a good grounding would be beneficial.

## Outline of signed division in hardware

An analysis of the microcode reveals that the 68000 performs signed division in hardware by

1. Taking the absolute values of the dividend and divisor
2. Checking to see if dividing the absolute values will overflow
3. Perform binary long division in a similar fashion to the unsigned routine
4. Restore the appropriate sign to the quotient
   1. If the dividend and divisor have the same sign, the quotient should be positive
   2. If the dividend and divisor have opposing signs, the quotient should be negative
   3. Checking that the final quotient has the appropriate sign and signalling overflow if not
5. Restore the appropriate sign to the remainder
   1. The 68000 goes with the convention that the remainder should have the same sign of the dividend

### Note on the long division algorithm

The unsigned division has to keep track of the previous most significant bit of the dividend. In the case
of signed division the algorithm is dividing two positive quantities and the most significant bit
of both the absolute dividend and absolute divisor is zero. There is therefore no need to keep track 
of the previous most significant bit.

This means that the signed division routine can be somewhat simplified.

### Pseudocode

The algorithm can be summarised as, for the number of bits in the divisor:

1. Shift the dividend left one bit
2. If the dividend is greater than or equal to the divisor
   1. Subtract the divisor from the dividend 
   2. Set the appropriate bit of the quotient to **1**
3. Otherwise,
   1. Set the appropriate bit of the quotient to **0**


The pseudocode below outlines the algorithm described above.

```c++
auto DivideSigned(uint32_t dividend, uint16_t divisor) -> uint32_t {
    if (divisor == 0u) { return 0u; } // Should signal division by zero
    const auto absDividend = AbsoluteValue(dividend);
    const auto absDivisor = AbsoluteValue(divisor);
    const auto absDiv16 = static_cast<uint32_t>(absDivisor) << 16u;
    if (dividend >= div16) { return 0u; } // Should signal overflow
    auto quotient = 0u;
    for (auto i = 0; i < 16; ++i) {
        dividend <<= 1u;
        quotient <<= 1u;
        if (dividend >= absDivs16) {
            dividend -= absDiv16;
            quotient += 1u;
        } else {
            quotient += 0u;
        }
    }
    return dividend + quotient;
}
```

## Signed division microcode

We now present the flowchart for the signed division microcode.

![DIVS Microcode](./figures/DIVS.svg "DIVS Microcode flowchart")

Glossary:

| Variable | Description                                   | Use                                               |
|----------|-----------------------------------------------|---------------------------------------------------|
| alu      | 16-bit ALU                                    | Initially holds the upper 16-bits of the dividend |
| alue     | 16-bit ALU Extender (Used by the shifter)     | Initially holds the lower 16-bits of the dividend |
| alub     | 16-bit ALU Buffer                             | Contains the 16-bit divisor throughout            |
| counter  | The combined two 16-bit Arithmetic Units (AU) | Used as the loop counter                          |
| temp     | 16-bit Address Temporary Low (ATL) register   | Used to remember dividends prior to subtraction   |
| temp2    | 16-bit Address Temporary High (ATH) register  | Used to remember remainder                        |

The flowchart is divided into three blocks. 

The first block performs the division by zero and overflow checks. This is longer than the unsigned variant
because it needs to take the absolute values of the dividend and divisor.

The second block performs the division of two positive numbers. This is somewhat simpler than the unsigned variant
since the most significant bit of the dividend and divisor are both zero and there's no longer the extra requirement
to remember the previous most significant bit.

The third block transfers the appropriate signs to the quotient and remainder. It also additionally checks for overflow.
It does this by checking that when a positive quotient is expected that the quotient received from the second block is 
positive and that when a negative quotient is expected that negating the received quotient produces a negative result.

### Summary

1. `DVS01` tests to see if the divisor is zero, positive or negative
2. `DVS03` provisionally negates the divisor
3. `DVS04` and `DVS05` test the sign of the dividend, `DVS05` additionally negates the divisor
4. `DVS06` provisionally negates the lower 16-bits of the dividend
5. `DVS10` negates the lower 16-bits of the dividend and negates the upper 16-bits
6. `DVS07` and `DVS11` check the absolute dividend and divisor for overlow
7. `DVS08` tests the most significant bit of the dividend
8. `DVS09` and `DVS0A` drive the loop counter and put the appropriate bit into the quotient
9. `DVS0C` subtracts the divisor from the dividend and check for loop termination
10. `DVS0C` and `DVS0E` are necessary to allow the give the processor time to make the flags available
11. `DVS0F` restores the previous dividend and tests its most significant bit
12. `DVS12` and `DVS13` put the appropriate bit into the quotient
    1. `DVS13` also updates the temporary with the last subtraction result
13. `DVS14` tests the sign of the original divisor
14. `DVS15` tests the sign of the original dividend
15. `DVS16` and `DVS1D` test the sign of the absolute quotient
16. Subsequent microwords are then concerned with:
    1. Assigning the correct signs to the quotient and remainder 
    2. And detecting overflow
    3. `DVS1D` and its successors deal with a negative divisor
    4. `DVS16` and its successors deal with a positive divisor

### Tracing 

The trace of the microcodes up to `DVS09` (the beginning of the loop) looks like this:

```c++
auto Cpu::DivideSigned(uint32_t divisor, uint16_t dividend) {
    MicroCycle(); // DVS01
    MicroCycle(); // DVS03
    if (divisor == 0) {
        // Hand over control to divide by zero trap
        return;
    }
    if (IsNegative(dividend)) {
        MicroCycle(); // DVS04/5
        MicroCycle(); // DVS06
        MicroCycle(); // DVS10
        MicroCycle(); // DVS11
        MicroCycle(); // DVS08
    } else {
        MicroCycle() // DVS04/5
        MicroCycle() // DVS06
        MicroCycle() // DVS07
        MicroCycle() // DVS08
    }
    if (AbsolueValue(divisor) / AbsoluteValue(dividend) >= 0x1'00'00u) {
        InternalCycle(); // DVUMZ
        BusCycle(); // DVUMA
        return;
    }
    // TODO loop
    // TODO sign correction
}
```

A possible trace through the loop is presented below

| Counter  | Remainder divisor relationship | Microword sequence                         |
|----------|--------------------------------|--------------------------------------------|
| 16 -> 15 | remainder >= divisor           | `DVS09` `DVS0C` `DVS0D`                    |
| 15 -> 14 | remainder <  divisor           | `DVS0A` `DVS0C` `DVS0D` `DVS0F`            |
| 14 -> 13 | remainder >= divisor           | `DVS09` `DVS0C` `DVS0D`                    |
| 13 -> 12 | remainder <  divisor           | `DVS0A` `DVS0C` `DVS0D` `DVS0F`            |
| 12 -> 11 | remainder >= divisor           | `DVS09` `DVS0C` `DVS0D`                    |
| 11 -> 10 | remainder >= divisor           | `DVS0A` `DVS0C` `DVS0D`                    |
| 10 -> 9  | remainder >= divisor           | `DVS0A` `DVS0C` `DVS0D`                    |
| 9 -> 8   | remainder >= divisor           | `DVS0A` `DVS0C` `DVS0D`                    |
| 8 -> 7   | remainder <  divisor           | `DVS0A` `DVS0C` `DVS0D` `DVS0F`            |
| 7 -> 6   | remainder >= divisor           | `DVS09` `DVS0C` `DVS0D`                    |
| 6 -> 5   | remainder >= divisor           | `DVS0A` `DVS0C` `DVS0D`                    |
| 5 -> 4   | remainder >= divisor           | `DVS0A` `DVS0C` `DVS0D`                    |
| 4 -> 3   | remainder >= divisor           | `DVS0A` `DVS0C` `DVS0D`                    |
| 3 -> 2   | remainder >= divisor           | `DVS0A` `DVS0C` `DVS0D`                    |
| 2 -> 1   | remainder >= divisor           | `DVS0A` `DVS0C` `DVS0D`                    |
| 1 -> 0   | *                              | `DVS0A` `DVS0C` `DVS0E` `DVS12/13` `DVS14` |
| 1 -> 0   | *                              | `DVS09` `DVS0C` `DVS0E` `DVS12/13` `DVS14` |

When the loop reaches the final iteration the current microword will be either is `DVS09` or `DVS0A`
and the processor executes the same number of microwords to reach `DVS14`. 

We can then continue the above method as follows

```c++
auto Cpu::DivideSigned(uint32_t divisor, uint16_t dividend) {
    MicroCycle(); // DVS01
    MicroCycle(); // DVS03
    if (divisor == 0) {
        // Hand over control to divide by zero trap
        return;
    }
    if (IsNegative(dividend)) {
        MicroCycle(); // DVS04/5
        MicroCycle(); // DVS06
        MicroCycle(); // DVS10
        MicroCycle(); // DVS11
        MicroCycle(); // DVS08
    } else {
        MicroCycle() // DVS04/5
        MicroCycle() // DVS06
        MicroCycle() // DVS07
        MicroCycle() // DVS08
    }
    if (AbsolueValue(divisor) / AbsoluteValue(dividend) >= 0x1'00'00u) {
        InternalCycle(); // DVUMZ
        BusCycle(); // DVUMA
        return;
    }
    const auto alignedDivisor = absDivisor << 16u;
    for (auto i = 0u; i < 15u; ++i) {
        MicroCycle(); // DVS09/A
        MicroCycle(); // DVS0C
        MicroCycle(); // DVS0D
        absDividend <<= 1u;
        if (absDividend >= alignedDivisor) {
            absDividend -= alignedDivisor;
        } else {
            MicroCycle(); // DVS0F
        }
    }
    MicroCycle(); // DVS09/A
    MicroCycle(); // DVS0C
    MicroCycle(); // DVS0E
    MicroCycle(); // DVS12/13
    MicroCycle(); // DVS14
    // TODO sign correction
}
```

For the final part we consider the traces from `DVS15` onwards for positive and negative dividends and divisors.

| Dividend | Divisor | Overflow | Microword sequence                              | Count |
|----------|---------|----------|-------------------------------------------------|-------|
| +        | -       | No       | `DVS15` `DVS1D` `DVS1F` `DVS20` `LEAA2`         | 5     |
| +        | -       | Yes      | `DVS15` `DVS1D` `DVS1F` `DVS20` `DVUMA`         | 5     |
| -        | -       | No       | `DVS15` `DVS1D` `DVS1E` `DVS1C` `LEAA2`         | 5     |
| .        | -       | Yes      | `DVS15` `DVS1D` `DVS1E` `DVUM4` `DVUMA`         | 5     |
| +        | +       | No       | `DVS15` `DVS16` `DVS17` `LEAA2`                 | 4     |
| +        | +       | Yes      | `DVS15` `DVS16` `DVS17` `DVUMA`                 | 4     |
| -        | +       | No       | `DVS15` `DVS16` `DVS1A` `DVS1B` `DVS1C` `LEAA2` | 6     |
| .        | +       | Yes      | `DVS15` `DVS16` `DVS1A` `DVS1B` `DVUM4` `DVUMA` | 6     |


When the divisor is negative is takes an additional 5 microwords to complete the instruction.

When the divisor is positive then we need to consider the sign of the dividend. When the dividend
is positive the processor needs 4 microwords to complete the instruction otherwise 6.

We can then complete the method as follows

```c++
auto Cpu::DivideSigned(uint32_t divisor, uint16_t dividend) {
    MicroCycle(); // DVS01
    MicroCycle(); // DVS03
    if (divisor == 0) {
        // Hand over control to divide by zero trap
        return;
    }
    if (IsNegative(dividend)) {
        MicroCycle(); // DVS04/5
        MicroCycle(); // DVS06
        MicroCycle(); // DVS10
        MicroCycle(); // DVS11
        MicroCycle(); // DVS08
    } else {
        MicroCycle() // DVS04/5
        MicroCycle() // DVS06
        MicroCycle() // DVS07
        MicroCycle() // DVS08
    }
    if (AbsolueValue(divisor) / AbsoluteValue(dividend) >= 0x1'00'00u) {
        InternalCycle(); // DVUMZ
        BusCycle(); // DVUMA
        return;
    }
    const auto alignedDivisor = absDivisor << 16u;
    for (auto i = 0u; i < 15u; ++i) {
        MicroCycle(); // DVS09/A
        MicroCycle(); // DVS0C
        MicroCycle(); // DVS0D
        absDividend <<= 1u;
        if (absDividend >= alignedDivisor) {
            absDividend -= alignedDivisor;
        } else {
            MicroCycle(); // DVS0F
        }
    }
    MicroCycle(); // DVS09/A
    MicroCycle(); // DVS0C
    MicroCycle(); // DVS0E
    MicroCycle(); // DVS12/13
    MicroCycle(); // DVS14
    if (SignBit(divisor)) {
        MicroCycle(); // DVS15
        MicroCycle(); // DVS1D
        MicroCycle(); // DVS1E/1F
        MicroCycle(); // DVS20/1C or DVUM4
    } else if (SignBit(dividend)) {
        MicroCycle(); // DVS15
        MicroCycle(); // DVS16
        MicroCycle(); // DVS1A
        MicroCycle(); // DVS1B
        MicroCycle(); // DVS1C or DVUM4
    } else {
        MicroCycle(); // DVS15
        MicroCycle(); // DVS16
        MicroCycle(); // DVS17
    }
    MicroCycle(); // LEAA2 or DVUMA
}
```

Note that this form isn't optimal and could, for performance purposes, be optimised.

## Conclusion

In this document we have presented how to binary division and how the 68000 implements it.

We then analysed the microcode to work out how many machine cycles it takes to perform signed division.
