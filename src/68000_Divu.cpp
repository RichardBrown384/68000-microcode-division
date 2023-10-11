#include "68000.h"

auto MC68000::ExecuteDivu() -> void {
    microword = DVUR1;
    while (true) {
        cycles += 2u;
        switch (microword) {
            case DVUR1: {
                // This mircoword sets up the test for division by zero
                pc = au;
                alue = rxdl; // lower 16-bits of dividend
                alub = rydl; // divisor
                ath = rydl; // divisor
                AluOp_AND(rydl, 0xFFFFu);
                microword = DVUM2;
                break;
            }
            case DVUM2: {
                // This microword sets up the overflow test
                // Callers: DVUR1
                const auto oldFlags = AluOp_SUB(rxdh, alub); // divisor - upper 16-bits of dividend
                microword = (oldFlags & FLAG_Z) ?
                            TRAP0 : // Division by zero handling
                            DVUM3; // Move to test msb of dividend, set up loop counter
                break;
            }
            case DVUM3: {
                // This microword sets up the loop counter and tests the msb of the dividend
                // Callers: DVUM2
                au = 15u + 1u; // loop counter
                atl = rxdh; // upper 16-bits of dividend
                const auto oldFlags = AluOp_AND(rxdh, 0xFFFFu); // upper 16-bits of dividend
                Print();
                microword = (oldFlags & FLAG_C) ?
                            DVUM5 : // Main division loop
                            DVUM4; // Overflow handling
                break;
            }
            case DVUM5: {
                // This microword shifts the dividend left 1 bit
                // and puts a 0 into the LSB of the dividend
                // Decrements the loop counter
                // Callers: DVUM3, DVUME
                au = au - 1u;
                const auto oldFlags = AluOp_SLAAx(0u);
                Print();
                microword = (oldFlags & FLAG_N) ?
                            DVUM7 : // If the most significant bit was a 1
                            DVUM8; // If the most significant bit was a 0
                break;
            }
            case DVUM6: {
                // This microword shifts the dividend left 1 bit
                // and puts a 1 into the LSB of the dividend
                // Decrements the loop counter
                // Callers: DVUM7, DVUMB
                au = au - 1u;
                const auto oldFlags = AluOp_SLAAx(1u);
                Print();
                microword = (oldFlags & FLAG_N) ?
                            DVUM7 : // If the most significant bit was a 1
                            DVUM8;  // If the most significant bit was a 0
                break;
            }
            case DVUM7: {
                // This microword subtracts the divisor from the upper 16 bits of the dividend
                // Callers: DVUM5, DVUM6
                atl = alu; // current remainder
                AluOp_SUB(alu, alub); // remainder - divisor
                microword = (au != 0) ?
                            DVUM6 : // Loop hasn't expired
                            DVUM9; // Loop has expired
                break;
            }
            case DVUM8: {
                // This microword subtracts the divisor from the upper 16 bits of the dividend
                // Note: this microword has the same nanoword origin as DVUM7
                // Callers: DVUM5, DVUM6
                atl = alu; // current remainder
                AluOp_SUB(alu, alub); // remainder - divisor
                microword = (au != 0) ?
                            DVUMB : // Loop hasn't expired
                            DVUMC; // Loop has expired
                break;
            }
            case DVUM9: {
                // This microcode copies the remainder from the alu back to the original register
                // And zeroes the alu
                // Callers: DVUM7
                rxdh = alu;
                AluOp_AND(alu, 0u);
                microword = DVUMD;
                break;
            }
            case DVUMB: {
                // This microcode is an idle wait
                // It's needed to give time for the DVUM8 flag evaluation to complete
                // Callers: DVUM8
                const auto oldFlags = flags;
                microword = (oldFlags & FLAG_C) ?
                            DVUME : // The divisor was greater than the dividend, restore old divisor
                            DVUM6; // The divisor was less than the dividend, 1 is required in the quotient
                break;
            }
            case DVUMC: {
                // This microcode copies the remainder from the alu back into the original register
                // and zeroes the alu
                // Note: this microword has the same nanoword origin as DVUM9
                // Callers: DVUM8
                rxdh = alu;
                const auto oldFlags = AluOp_AND(alu, 0u);
                microword = (oldFlags & FLAG_C) ?
                            DVUMF : // The last subtraction produced carry, so we need to fix up the remainder
                            DVUMD; // No need to fix up the remainder
                break;
            }
            case DVUMD: {
                // This microcode shifts left putting a 1-bit into the lsb of the alu extender
                // It initiates the next instruction read
                // Callers: DVUM9, DVUMC
                au = pc + 2u;
                alub = alu;
                AluOp_SLAAx(1u);
                microword = DVUM0;
                break;
            }
            case DVUME: {
                // This microword restores the previous dividend, setting the N flag
                // Callers: DVUMB
                AluOp_AND(atl, 0xFFFFu);
                microword = DVUM5;
                break;
            }
            case DVUMF: {
                // This mircoword restores the previous dividend to rx
                // and shifts a zero into the least significant bit of the quotient
                // Callers: DVUMC
                au = pc + 2u;
                alub = alu;
                rxdh = atl;
                AluOp_SLAAx(0u);
                microword = DVUM0;
                break;
            }
            case DVUM0: {
                // this microword reads the next instruction word
                // And sets the flags
                // Callers: DVUMD, DVUMF
                rxdl = alue;
                AluOp_SUB(alue, alub);
                microword = A1;
                break;
            }
            case DVUM4: {
                // overflow detected
                // sets up the program counter for read
                microword = DVUMA;
                break;
            }
            case DVUMA: {
                microword = A1;
                break;
            }
            case TRAP0: // division by zero
            case A1: // control has been returned to next macro instruction
            default: {
                cycles -= 2u; // Discount these cycles
                return;
            }
        }
    }
}