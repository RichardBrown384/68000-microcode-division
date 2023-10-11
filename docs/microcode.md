# 68000 microcode primer

## Table of Contents

1. [Introduction](#introduction)
2. [68000 internals](#68000-internals)
3. [Macrocode, microcode and nanocode](#macrocode-microcode-and-nanocode)
4. [Microword blocks](#microword-blocks)
5. [Summary](#summary)

## Introduction

The purpose of the document is to try and succinctly explain parts of USP 4,325,121 so that the workings of individual
instructions may be analysed.

I originally wanted to investigate the inner workings of the `DIVU` and `DIVS` so I could understand the cycle counts
involved. This required an understanding of the patent and obtaining that understanding was involved
process.

This document very probably contains errors and mistakes. The patent has several pages that are hardly legible and
likely contain errors and, the patent details the workings of a prototype and not the final product.

## 68000 internals

Before diving into the details of the microcode it is probably best to summarise here some details 
of the inner workings of the 68000.

The 68000 programmer's model exposes 8 32-bit data registers, 8 32-bit address registers, two stack pointers and a
status register (usually conceptually split into the condition code register and the supervisor section).

In order to actually operate the 68000 has two 16-bit arithmetic units and one 16-bit arithmetic logic unit. The
arithmetic and arithmetic logic units are connected to two buses, the address bus and the data bus, that are conceptually
divided into three regions; data, low and high.

![Alu and AU](./figures/Alu%20and%20Au.svg)

### Arithmetic Unit High

The High Arithmetic Unit looks like

![Arithmetic Unit High](./figures/Arithmetic%20Unit%20High.svg)

with lines indicating bidirectional links and arrows unidirectional ones.

This introduces the upper 16-bits of two temporary registers. The Address and Data Temporary Registers which are used
as storage during various calculations.

There's a constants ROM which is used to load values like 1 and 2 into the arithmetic unit without requiring
bus access.

The Arithmetic Unit can also write back to the Program Counter without going via either bus.

### Arithmetic Unit Low

The Low Arithmetic Unit looks like

![Arithmetic Unit Low](./figures/Arithmetic%20Unit%20Low.svg)

Present and correct are the lower 16-bits of the program counter and the lower 16-bits of the address registers.

It's also here we find the lower 16-bits of the Address Temporary Register.

Like the Arithmetic Unit High, the Low Arithmetic Unit can write back directly to the Program Counter
without accessing the bus.

The 68000 is a pipelined processor and prefetches instructions ahead of the Program Counter visible to the programmer.
Since the instructions are prefetched there are three additional internal registers for instruction retrieval and
decoding, IRC, IR and IRD.

The Field Translation Unit is responsible for decoding macro instruction fields stored in IRD (e.g. source and
destination registers) or supplying certain constants.

### Arithmetic Logic Unit

The Arithmetic Logic Unit looks like

![Arithmetic Logical Unit](./figures/Arithmetic%20Logic%20Unit.svg)

The lower 16-bits of the Data Temporary Register reside here as do the lower 16-bits of the data registers.

The Bit Manipulation Decoder (DCR) is beyond the scope of the document but included for purposed of completeness.

The two novel features in this unit are the ALU Extender (ALUE) and the ALU Buffer (ALUB).

The ALU Extender is used by bitwise shift operations. For example, the upper 16-bits of a register may be loaded into
the ALU and the lower 16-bits of said register might be loaded into the extender and when a shift left operation is
requested, the ALU extender is used to supply the next least significant bit for the ALU without going via the
Data Bus.

The ALU Buffer is used to store and supply 16-bit values without going via either bus. For example, during division
the ALU Buffer is used to store the 16-bit divisor.

## Macrocode, microcode and nanocode

The 68000 effectively has not one, but three instruction sets. The macrocode, the microcode and the nanocode. They
perform different roles within the processor.

### Macrocode

The macrocode is the instruction set one uses to program the 68000. For example, `ADD.W D1,D2` and
`ANDI.W #1212, D1`.

Each macrocode can be associated with up to 3 microcode words. The relationships are set out in full in Figure 21
of the patent. We present a small portion of the table transposed as an example.

| Macrocode     | a1    | a2    | a3    |
|---------------|-------|-------|-------|
| DIVU   Dy, Dx | dvur1 |       |       |
| DIVU   Ay, Dx | trap1 |       |       |
| DIVU (Ay), Dx | adrw1 |       | dvum1 |
| DIVU #imm, Dx | e#w1  | dvur1 |       |

When the processor begins executing an instruction it executes the microcode in the `a1` position. Subsequent 
microwords call other microwords until they request that the microword in the `a1` position be executed and 
control is passed to the next instruction.

While executing it's possible for a microword to request that the microwords in the `a2` or `a3` positions be executed.
This allows the processor to have the microword placed in the `a1` to deal with the requested address mode and then
the microword in the `a2` or `a3` position to perform the action requested.

Summarizing the table:

`DIVU Dy, Dx` begins executing `dvur1` and doesn't return until the action completes.

`DIVU Ay, Dx` is an illegal instruction and it simply executes `trap1`.

`DIVU (Ay), Dx` begins by executing `adrw1` which executes `adrw2` which requests the microword in `a3` be executed
which initiates `dvum1`.

`DIVU #imm, Dx` begins by executing `e#w1` to read the immediate word which upon completion requests the microword 
in `a2` be executed which initiates `dvur1`.

Appendix A of the patent tabulates the microwords with their branch behaviours in more detail.

### Microcode

The 68000 has 544 17-bit microwords stored in ROM. Each microword is associated with a nanoword. As there are more
microwords than nanowords the relationship between nanowords and microwords is one-to-many. That is to say it is
possible for a nanoword to be associated with more than one microword. The microwords are assigned labels such
as `dvur1`and `dvum1` (divide unsigned register 1 and divide unsigned memory 1 respectively).
 
Since the nanowords do not contain any conditional logic the branching between various microwords is
handled at this level in the CPU. For example, `dvum2`, the microcode responsible for checking
for division by zero, jumps to `dvur2` if the`Z` flag is true and `dvum3` otherwise.

There are two kinds of microword, those that branch unconditionally (Format I) and those that branch conditionally (
Format II).

#### Format I

Format I microwords branch unconditionally and are encoded as

| 16-15              | 14-5                         | 4   | 3-2  | 1   | 0                    |
|--------------------|------------------------------|-----|------|-----|----------------------|
| Function Code (FC) | Next Micro ROM Address (NMA) | C   | Type | 0   | Load Instruction (I) |

The Function Code can take one of 5 possible values listed in the table below. They essentially describe how the 68000
should interact with the bus while executing a microcode word.

| Function Code (FC) | Meaning               |
|--------------------|-----------------------|
| n (00)             | No access             |
| u (00)             | Unknown access type   |
| d (01)             | Data access           |
| i (10)             | Instruction access    |
| a (11)             | Acknowledge interrupt |

`n` is used when the processor is performing some internal activity like using the ALU.

`i` or `d` are used when the processor is fetching instruction or data from the bus.

The Next Micro ROM Address is the address of the next micro word to execute and how this is interpreted depends on the
Type field

| Type    | Meaning              |
|---------|----------------------|
| db (00) | NMA (direct branch)  |
| a1 (01) | Address 1            |
| a2 (10) | Address 2            |
| a3 (11) | Address 3            |

When the Type is `db` then the NMA field is used unconditionally otherwise one of the addresses specified by the
[macrocode](#macrocode) `a1`, `a2` or `a3` entries is to be used.

The C and I fields are interpreted together as

| Abbreviation | C   | I   | Meaning                           |
|--------------|-----|-----|-----------------------------------|
| db           | 0   | 0   | Don't update IR, don't update TVN |
| dbi          | 0   | 1   | Update IR, update TVN             |
| dbc          | 1   | 0   | Update IR, don't update TVN       |
| dbl          | 1   | 1   | Update IR, partially update TVN   |

Updating IR means moving IRC into IR and updating TVN means updating the Trap Vector Number.

#### Format II

Format II microwords branch conditionally and are encoded as

| 16-15              | 14-7                               | 6-2                             | 1   | 0                    |
|--------------------|------------------------------------|---------------------------------|-----|----------------------|
| Function Code (FC) | Next Micro ROM Base Address (NMBA) | Conditional Branch Choice (CBC) | 1   | Load Instruction (I) |

The Function Code Field is the same as that for [Format I](#format-i) microwords.

The Next Micro ROM Base Address by itself isn't sufficient to determine the address of the next microword. It has to be
augmented by the branch circuitry. The full discussion of this is beyond the scope of this document but one imagines
that the base address points to a location in the nanoword ROM and depending on the outcome of the conditional test an
offset is added to this location to get the next nanoword.

The Conditional Branch Choice is summarized here, the table in full can be found in Appendix C in the patent. The full
explanation for all of these is beyond the scope of the document.

| CBC         | Source    | Meaning                                                                                   |
|-------------|-----------|-------------------------------------------------------------------------------------------|
| i11         | IRC       | Tests if bit 11 of the extension word is set (the W/L bit)                                |
| auz         | AU        | Tests if the Arithmetic Unit is zero (can be used as a loop counter)                      |
| c           | PSW (CCR) | Tests if the carry bit is clear or set                                                    |
| z           | PSW (CCR) | Tests if the zero bit is clear or set                                                     |
| nz1 (n ^ z) | PSW (CCR) | Tests if the alu result is either zero or negative (used for testing divisor in division) |
| n           | PSW (CCR) | Tests if the negative bit is set                                                          |
| nz2 (n ^ z) | PSW (CCR) |                                                                                           |
| mx0         | AU,IRD    |                                                                                           |
| m10         | AU,IRD    |                                                                                           |
| ze          | AU        |                                                                                           |
| nv  (n ^ v) | PSW (CCR) |                                                                                           |
| d4          | DCR       | For bit operations if bit 4 of the bit number is set (e.g. are we testing bits 16-31?)    |
| v           | PSW (CCR) | Tests if the overflow bit is set                                                          |
| en1         | AU,IRD    |                                                                                           |
| cc          | PSW (CCR) |                                                                                           |

The I field is interpreted as

| Abbreviation | I   | Meaning         |
|--------------|-----|-----------------|
| bc           | 0   | Don't update IR |
| bci          | 1   | Update IR       |


### Nanocode

The 68000 has 336 68-bit nanowords stored in ROM. At a very high level these instruct the 68000 which data to move into
the ALU (source and destination), which ALU operation to perform and so on. At this level there is no control flow
logic.

Here's a redacted summary of the terms used in the patent nanocode and a short explanation
of their purpose

| Term         | Meaning                                                                              |
|--------------|--------------------------------------------------------------------------------------|
| alu          | Arithmetic Logic Unit                                                                |
| alub         | Arithmetic Logic Unit Buffer (used as an accumulator)                                |
| alue         | Arithmetic Logic Unit Extender (used to manage the lower 16-bits for shifts)         |
| au           | Arithmetic Unit                                                                      |
| edb          | External Data Bus (read or write from external memory)                               |
| dbin         | Data Bus Input Buffer (contains data read by the processor)                          |
| db           | Data Bus                                                                             |
| db{h,l,d}    | Data Bus {high, low, data}                                                           |
| dob          | Data Bus Output Buffer                                                               |
| ab           | Address Bus                                                                          |
| ab{h,l,d}    | Address Bus {high, low, data}                                                        |
| ab           | Address Bus                                                                          |
| ab{h,l,d}    | Address Bus {high, low, data}                                                        |
| aob          | Address Bus Output Buffer                                                            |
| dt           | Temporary Data register                                                              |
| at           | Temporary Address register                                                           |
| r{x,y}       | Register expressed by the x and y instruction fields                                 |
| r{x,y}d      | Data register expressed by the x and y instruction fields                            |
| r{x,y}a      | Address register expressed by the x and y instruction fields                         |
| rz           | The Index Register                                                                   |
| ftu          | Field Translation Unit (extract fields from an instruction, provide constants, etc). |
| tvn          | Trap Vector Number                                                                   |
| dcr          | Bit Manipulation Decoder                                                             |
| psw          | Program Status Word (the condition code register)                                    |
| ssw          | Supervisor Status Word                                                               |
| irc, ir, ird | Instruction decoding registers                                                       |

The [section](#68000-internals) covers the meaning of the majority of these terms.

The nanocode itself then comprises statements like

```txt
rxdh -> db -> alu
```

Which means move the upper 16-bits of the destination data register (usually, but not always, encoded as Rx in the
macrocode instruction) to the Arithmetic Logic Unit via the Data Bus.

## Microword blocks

This section tries to collect all the information about the Microword blocks listed in the patent in one place.

Appendix H of the patent details each microcode individually with typed blocks in the following format

```text
+-------------------------------------+---------------------------+
| Nanocode                            | Access label              | 
|                                     |---------------------------|
|                                     | Next word row address     |
|                                     |---------------------------|
|                                     | ALU function              |
|                                     |---------------------------|
|                                     | Register pointers         |
+-------------------------------------+---------------------------+
| Micro Row Address | Microword Label | Origin                    |
+-----------------------------------------------------------------+
                            |
                            V
                      Branch address
```

The fields can be interpreted as follows

| Field             | Description                                                                              |
|-------------------|------------------------------------------------------------------------------------------|
| Nanocode          | The nanocode associated with this microcode                                              |
| Micro Row Address | The address of the microcode, not important for interpretation                           |
| Microword Label   | The label of this microcode                                                              |
| Origin            | The address of the nanocode associated with the microcode                                |
| Access label      | [Access label](#access-label)                                                            |
| Next Row Address  | One of the microword branch types, e.g. `db`, `dbi` or `bc`                              |
| ALU function      | The [ALU function](#alu-function) to perform, varies by macro instruction type           |
| Register pointers | The [Register pointers](#register-pointers) understood not to be present in the nanocode |

### Access label

#### Access class

The first character of the access label is the access class, and it may be one of

| Character | Description |
|-----------|-------------|
| i         | initiate    |
| f         | finish      |
| n         | no access   |
| t         | total       |

and it describes the kind of external data bus access the processor is to undertake.

For the purposes of cycle counting all types take 1 processor microcycle (2 clocks) except `t` which requires 2
processor microcycles (4 clocks. The first 2 clocks initiate the bus cycle, the last two finish it).

#### Access mode

The second character of the access label is the access mode, and it may be one of

| Character | Description |
|-----------|-------------|
| p         | process     |
| r         | read        |
| w         | write       |

and it describes how the microcode interacts with the external data bus.

The character combination `np` (no access, process) is used for internal processor operations where no access to the bus
is required.

#### Access type

The final two characters of the access label (if present) are the access type, and they may be

The second character of the access label is the access mode, and it may be one of

| Characters | Description              |
|------------|--------------------------|
| ak         | Acknowledge interrupt    |
| im         | Immediate                |
| in         | Instruction              |
| ix         | Instruction or immediate |
| op         | Operand                  |
| uk         | Unknown                  |

and it describes how the microcode interacts with the external data bus.

### ALU function

There's quite a bit to digest here. Let's begin by presenting an excerpt of Figure 17 from the patent

| Row | Column 1              | Column 2   | Column 3   | Column 4    | Column 5 | Instructions       |
|-----|-----------------------|------------|------------|-------------|----------|--------------------|
| 1   | AND -NZ00             | SUB -NZ0C' | SUBX       | SLAAx (LSS) | EXT      | DIVU, DIVS         |
| 2   | AND -NZ00 # AND -NZ-- | SUB CNZVC  | ADDX CNZVC | ASR ANZ0A   | EXT      | ADD, ADDI, ADDQ... |

This figure tabulates for which kind of instruction, given a column specifier, the operation the ALU is to perfom.

**Note:** `C'` means the complement of carry and `A is the output from the shifter

The ALU function itself comprises one, two or three characters. Paraphrasing the patent, which contains
important information for division buried in a wall of text,

The first character indicates the column of the table shown in Figure 17 which is to be selected in order to perform the
desired operation. The symbols 1-5 correspond to columns 1 through 5 in this table. The symbol **x** indicates a don't
care condition. In addition, the symbol 6, which occurs in microword blocks used to perform a divide algorithm,
indicates that column 4 is enabled but that the `LSS` ALU function will shift a logic '1' into the least significant
bit of the ALUE, as opposed to '0' as is the case when '4' appears.

If two or more characters appear then the second character refers to the finish and initiate identification for
condition code control. The symbol **f** corresponds to finish and would therefore cause the second entry in column 1 of
the table shown in Figure 17 to be used in order to control the setting of the condition code bits in the PSW register.
The symbol **i** specifies initiate and would select the first entry in column 1 of the table 40 shown in Figure. 17 for
controlling the setting of the condition code bits in the PSW register. The symbol **n** indicates that the condition
codes are not affected for the particular operation.

Finally, for those microword blocks which contain a three character code, the third character is the symbol **f** which
indicates to the execution unit that a byte transfer is involved.

**Note:** this section of the patent refers to both Figures 17 and 18, but I think they mean 17 throughout. Figure 18
looks like it has something to do with how the instruction bits are decoded.

In conclusion, when looking at microwords for the division routines we need to look in the first row for the appropriate 
ALU Function to invoke and when we're doing addition we need to look at the second row.

### Register pointers

If present, the register pointers are a 4 character block.

The first two characters specify the destination

| Characters | Destination                       |
|------------|-----------------------------------|
| dt         | Data temporary register           |
| dx         | Don't care                        |
| rx         | Rx field in the macro instruction |
| sp         | User or supervisor stack pointer  |
| uk         | Unknown                           |
| us         | User stack pointer                |

The final two characters specify the source

| Characters | Destination                       |
|------------|-----------------------------------|
| dt         | Data temporary register           |
| dy         | Don't care                        |
| pc         | Program counter                   |
| ry         | Ry field in the macro instruction |
| py         | Program counter or Ry field       |
| uk         | Unknown                           |

For the purposes of our analysis, this field isn't of much interest.

## Microword example

This section details, for demonstration purposes, the workings of the first two microwords of the `DIVU` instruction.

Let's consider for a moment the macro instruction `DIVU Dy, Dx` that begins execution with `dvur1` and tries to divide
Dx by the lower 16-bits of Dy.

```text
+----------------------------------+--------+
| au -> pc                         | np     |  
| (rxdl) -> dbd -> alue            |--------|  
| (rydl) -> ab -> alu, alub, ath   | db     |  
| -1 -> alu                        |--------|  
|                                  | 1i     |
|                                  |--------|  
|                                  | rxuk   |  
|----------------------------------|--------|  
| 17a | dvur1                      | dvur1  |  
+----------------------------------+--------+  
                    |                                                                            
                    v                      
+----------------------------------+--------+
| (alub) -> alu                    | np     | 
| (rxh) -> db -> alu               |--------|
|                                  | bc     |
|                                  |--------|
|                                  | 2i     |
|                                  |--------|
|                                  |        |
+----------------------------------+--------+
| 21e | dvum2                      | dvum2  |
+----------------------------------+--------+
                   |
                   +------> dvum3 if not z
                   |
                   +------> dvur2 if z
```

### dvur1

Let's examine the tasks the associated nanoword performs.

* `au -> pc`: This copies the contents of the address unit into the program counter
* `(rxdl) -> dbd -> alue`: This copies the lower word of Dx to the ALU Extender via the data bus
* `(rydl) -> ab -> alu, alub, ath`: This copies the lower word of Dy via the address bus to the
    * ALU (first operand),
    * ALU Buffer and,
    * Higher word of the temporary address register
* `-1 -> alu`: Copies -1 into the ALU (second operand)

Consulting the extract of Figure 17 above, the ALU Function **1i**, means that the ALU operation to perform is
`AND -NZ00`. In the context of division we're trying to work out if the divisor (lower word of Dy) is zero by ANDing it
together with -1 (all bits set). If the result is zero, then the divisor is zero.

Interestingly, the nanoword performs the logical test, but it doesn't appear to be used yet.

`dvur1` is a direct branch microword so execution immediately proceeds to `dvur2`

### dvur2

Let's repeat the exercise, and examine the nanoword actions

* `(alub) -> alu`: Moves the contents of the ALU Buffer (the divisor) into the ALU as the first argument
* `(rxh) -> db -> alu`: Moves the upper word of register Dx into the ALU as the second argument via the data bus

The ALU function is **2i** meaning that the ALU performs `SUB -NZ0C'`. This is essentially the overflow check. If
the upper 16-bits of the dividend are greater than or equal to divisor then overflow occurs.

However, this macroword has a branch condition associated with it. If the result of the last operation was zero
then the processor is to initiate the exception handling process at `dvur2`. Otherwise, execution continues to
`dvum3`.

## Summary

In this document we've attempted to summarise the workings of the 68000 patent micro- and nanocode with the goal
of presenting the necessary information in one place.
