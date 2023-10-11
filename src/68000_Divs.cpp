#include "68000.h"

auto MC68000::ExecuteDivs() -> void {
    microword = DVS01;
    while (true) {
        cycles += 2u;
        switch (microword) {
            /*
             * Division by zero, take absolute values, check for unsigned overflow
             */
            case DVS01: {
                // Sets up test for division by zero and checking sign of divisor
                pc = au;
                alue = rxdl; // lower 16-bits of dividend
                alub = rydl; // 16-bit divisor
                ath = rydl; // 16-bit divisor
                AluOp_AND(rydl, 0xFFFFu);
                microword = DVS03;
                break;
            }
            case DVS03: {
                // Subtracts the divisor from zero, so we can take the absolute value
                // Branches to a trap for zero dividend, or different microwords
                // depending on the sign of the divisor
                // Callers: DVS01
                const auto oldFlags = AluOp_SUB(0, alub); // subtract divisor from zero
                microword = (oldFlags & FLAG_Z) ?
                            TRAP0 : // division by zero
                            (oldFlags & FLAG_N) ?
                            DVS05 : // Negative divisor
                            DVS04;  // Positive divisor
                break;
            }
            case DVS04: {
                // This microword sets up the loop counter and tests the msb of the dividend
                // Note: uses the same nanoword as DVUM3
                // Callers: DVS03 (positive divisor)
                au = 15u + 1u; // loop counter
                atl = rxdh; // upper 16-bits of dividend
                AluOp_AND(rxdh, 0xFFFFu); // upper 16-bits of dividend
                microword = DVS06;
                break;
            }
            case DVS05: {
                // This microword sets up the loop counter and tests the msb of the dividend
                // And negates a negative divisor
                // Callers: DVS03 (negative divisor)
                au = 15u + 1u; // loop counter
                atl = rxdh; // upper 16-bits of dividend
                alub = alu; // update alub with negated (i.e. now positive) divisor
                AluOp_AND(rxdh, 0xFFFFu); // upper 16-bits of dividend
                microword = DVS06;
                break;
            }
            case DVS06: {
                // Microword negates the lower bits of the dividend
                // Callers: DVS04, DVS05
                const auto oldFlags = AluOp_SUB(0u, rxdl);
                microword = (oldFlags & FLAG_N) ?
                            DVS10 : // Negative dividend
                            DVS07; // Positive dividend
                break;
            }
            case DVS07: {
                // Microword sets up the overflow test when the dividend was positive
                // Callers: DVS06
                AluOp_SUB(atl, alub); // upper 16-bits of dividend - divisor
                microword = DVS08;
                break;
            }
            case DVS08: {
                // Microword sets the N flag for the MSB of the absolute dividend
                // Callers: DVS07
                const auto oldFlags = AluOp_AND(atl, 0xFFFFu);
                Print();
                microword = (oldFlags & FLAG_C) ?
                            DVS09 : // Main division loop
                            DVUMZ; // Overflow handling
                break;
            }
            case DVS10: {
                // Microword continues the process of negating a negative dividend
                alue = alu; // Dividend was negative so move absolute lower 16-bits into alu extender
                AluOp_SUBX(0u, rxdh); // Negate upper bits of dividend
                microword = DVS11;
                break;
            }
            case DVS11: {
                // Microword sets up the overflow test when the dividend was negative
                // Callers: DVS10
                atl = alu; // We want the absolute dividend stored in atl
                AluOp_SUB(atl, alub); // upper 16-bits of dividend - divisor
                microword = DVS08;
                break;
            }

                /*
                 * Main division loop
                 */
            case DVS09: {
                // Logical shift left with 0 into lsb
                // Decrement counter
                // Callers: DVS08, DVS0F
                au = au - 1;
                AluOp_SLAAx(0u);
                Print();
                microword = DVS0C;
                break;
            }
            case DVS0A: {
                // Logical shift left with 1 into lsb
                // Decrement counter
                // Callers: DVS0D
                au = au - 1;
                AluOp_SLAAx(1u);
                Print();
                microword = DVS0C;
                break;
            }
            case DVS0C: {
                // Subtracts divisor from dividend
                // Callers: DVS09, DVS0A
                atl = alu; // Remember the current dividend/remainder
                AluOp_SUB(alu, alub);
                microword = (au != 0) ?
                            DVS0D : // Loop hasn't expired
                            DVS0E; // Loop has expired
                break;
            }
            case DVS0D: {
                // Idle wait
                // Callers: DVS0C
                const auto oldFlags = flags;
                microword = (oldFlags & FLAG_C) ?
                            DVS0F : // Restore previous dividend/remainder
                            DVS0A; // Put 1 into the quotient
                break;
            }
            case DVS0F: {
                // Restores the previous dividend
                AluOp_AND(atl, 0xFFFFu);
                microword = DVS09;
                break;
            }
            case DVS0E: {
                // Idle wait
                // Callers: DVS0C
                const auto oldFlags = flags;
                microword = (oldFlags & FLAG_C) ?
                            DVS12 : // least significant bit of quotient is 0
                            DVS13; // leas significant bit of quotient is 1
                break;
            }
            case DVS12: {
                // Sets the least significant bit of the quotient to 0
                // Callers: DVS0E
                AluOp_SLAAx(0u);
                Print();
                microword = DVS14;
                break;
            }
            case DVS13: {
                // Sets the least significant bit of the quotient to 1
                // Overwrites the address temporary low with the correct remainder
                // Callers: DVS0E
                atl = alu;
                AluOp_SLAAx(1u);
                Print();
                microword = DVS14;
                break;
            }

                /*
                 * Tests the signs of the original divisor and dividend to fix
                 * quotient and remainder signs
                 */
            case DVS14: {
                // Tests the sign of the original divisor
                // Callers: DVS12, DVS13
                AluOp_AND(ath, 0xFFFFu);
                Print();
                microword = DVS15;
                break;
            }
            case DVS15: {
                // Tests the sign of the original dividend
                // Move quotient from alue into alub
                // Callers: DVS14
                alub = alue;
                const auto oldFlags = AluOp_AND(rxdh, 0xFFFFu);
                microword = (oldFlags & FLAG_N) ?
                            DVS1D : // Negative divisor (< 0)
                            DVS16; //  Positive divisor (>= 0)
                break;
            }
            case DVS16: {
                // Positive divisor: Test sign of quotient
                // Callers: DVS15
                ath = atl; // Move remainder into address temporary high
                const auto oldFlags = AluOp_AND(alub, 0xFFFFu);
                microword = (oldFlags & FLAG_N) ?
                            DVS1A : // Positive divisor, negative dividend
                            DVS17; // Positive divisor, positive dividend
                break;
            }
            case DVS1D: {
                // Negative divisor: Test sign of quotient
                // Callers: DVS15
                ath = atl; // Move remainder into address temporary high
                const auto oldFlags = AluOp_AND(alub, 0xFFFFu);
                microword = (oldFlags & FLAG_N) ?
                            DVS1E : // Negative divisor, negative dividend
                            DVS1F; // Negative divisor, positive dividend
                break;
            }

                /*
                 * Positive divisor, positive dividend
                 */
            case DVS17: {
                // Computes final set of flags
                // Callers: DVS16
                atl = alu;
                const auto oldFlags = AluOp_AND(alub, 0xFFFFu);
                microword = (oldFlags & FLAG_N) ?
                            DVUMA : // Negative quotient, should be positive: overflow
                            LEAA2;
                break;
            }

                /*
                * Positive divisor, negative dividend
                */
            case DVS1A: {
                // Negates the quotient (since dividend and divisor have opposing signs)
                // Callers: DVS16
                AluOp_SUB(0u, alub);
                microword = DVS1B;
                break;
            }
            case DVS1B: {
                // Negates the remainder (since remainder and dividend are to have the same sign)
                // Callers: DVS1A
                alub = alu; // Update alub with negated quotient
                atl = alu;
                const auto oldFlags = AluOp_SUB(0u, ath);
                microword = ((oldFlags & (FLAG_N | FLAG_Z)) == 0u) ?
                            DVUM4 : // Positive quotient (> 0) when we expected a negative one, overflow
                            DVS1C; // quotient is less than or equal to zero, proceed as normal
                break;
            }
            case DVS1C: {
                // Computes final set of flags
                // Callers: DVS1B, DVS1E
                ath = alu; // Update remainder with negated copy
                AluOp_AND(alub, 0xFFFFu);
                microword = LEAA2;
                break;
            }

                /*
                 * Negative divisor, positive dividend
                 */
            case DVS1F: {
                // Negates the quotient (since divisor and dividend have opposing signs)
                // Callers: DVS1D
                AluOp_SUB(0u, alub);
                microword = DVS20;
                break;
            }
            case DVS20: {
                atl = alu; // Update quotient with negated value
                // Note: this isn't stated in the microde listing, but then the final flags would be wrong?
                alub = alu; // Update quotient with negated value
                const auto oldFlags = AluOp_AND(alub, 0xFFFFu);
                microword = ((oldFlags & (FLAG_N | FLAG_Z)) == 0u) ?
                            DVUMA : // Negated quotient is positive, and we expected a negative one, overflow
                            LEAA2;
                break;
            }

                /*
                 * Negative divisor, negative dividend
                 */
            case DVS1E: {
                // Negate the remainder, since remainder has the same sign as the dividend
                // Callers: DVS1D
                alub = alu; // move quotient into alu buffer
                atl = alu; // Move quotient into address temporary low
                const auto oldFlags = AluOp_SUB(0u, ath);
                microword = (oldFlags & FLAG_N) ?
                            DVUM4 : // Divisor and dividend have opposing signs, quotient is negative, expected positive, overflow
                            DVS1C;
                break;
            }

                /*
                 * Write the results back to the registers,
                 * prepare to return control to the next macro instruction
                 */
            case LEAA2: {
                // Callers: DVS17, DVS1C, DVS20
                rxdh = ath;
                rxdl = atl;
                microword = A1;
                break;
            }
                /*
                 * Exits
                 */
            case DVUM4: [[fallthrough]];
            case DVUMZ: {
                // overflow detected
                // sets up the program counter for read
                microword = DVUMA;
                break;
            }
            case DVUMA:  {
                microword = A1;
                break;
            }
            case TRAP0: [[fallthrough]]; // division by zero
            case A1: // control has been returned to next macro instruction
            default: {
                cycles -= 2u; // Discount these cycles
                return;
            }
        }
    }
}