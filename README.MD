# 68000 Microcode Division

## About

This repository contains an analysis of the 68000 `DIVU` and `DIVS` instructions at the micro- and nanocode level
as presented in US patents 4325121 and 4296469.

The motivation was to understand the correct timings for the instructions in question.

The repository comprises

1. A trimmed down 68000 microcode emulator for the `DIVU` instruction and associated unit tests
2. A trimmed down 68000 microcode emulator for the `DIVS` instruction and associated unit tests
3. Notes outlining the basics of 68000 micro- and nanocodes
4. Notes outlining the workings of the `DIVU` and `DIVS` instructions

## Code

This project uses CMake as the build tool and googletest for testing. To build and run the tests:

``` bash
mkdir my_build_directory
cd my_build_directory
cmake -G <your favourite generator> /path/to/this/repository
cmake --build .
ctest --verbose
```

## Notes

The notes in this repository are presented in the suggested reading order

1. [68000 microcode primer](./docs/microcode.md)
2. [68000 unsigned division](./docs/unsigned-division-algorithm.md)
3. [68000 signed division](./docs/signed-division-algorithm.md)
4. [Bibliography](./docs/bibliography.md)

## 3rd party documents

The documents in the `docs-third-party` folder are copyright their original authors. 

In particular,
[the searchable document for the US patent appendix](./docs-third-party/US4325121%20-%20Appendix%20F%20Microcode.txt)
is the work of Exodus Megadrive emulator (Nemesis), without which this project would have taken longer.

The documents are included here for preservation purposes only.

# Licence and copyright

68000 Microcode Division is licensed under MIT. Copyright 2023 Richard Brown.