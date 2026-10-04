# First Assembly Program: Morse

First assembly-language program of the course: a Morse code beacon that blinks all
4 LEDs of the board at once, spelling "1 2 3 4" (or any other message) in Morse code.
Written from scratch in assembly for a bare-metal RV32I CPU (no multiply/divide, no
compressed instructions, 1 KB of RAM), for the FemtoRV32 SoC on the Tang Primer 20K
FPGA board.

## Files

- `morse.s` — the program itself: a lookup table encodes each character's Morse
  pattern in a single byte (a sentinel bit marks the length), and the main loop
  walks it with a bit mask instead of shifts (`slli`/`srli` are avoided — see note
  below), toggling the LED register and timing each dot/dash with a calibrated
  delay loop.
- `drivers/gpio.c`, `drivers/gpio.h` — writes to the memory-mapped LED register.
- `drivers/uart.c`, `drivers/uart.h` — UART driver (not used by this program, kept
  for reuse by later assignments).
- `system/boot.s`, `system/link.ld` — startup code and the 1 KB memory layout.

## A hardware bug found along the way

Shifts were avoided on purpose: in simulation, the CPU's shifter was found to shift
one position short of what it should (`slli`/`srli` by *k* positions behave as *k-1*).
The full writeup, the test that reproduces it, and a one-line proposed fix are in
the monthly lab report, built on top of the
[`Femto_Risc-V_SoC`](https://github.com/Sbustamantem/Femto_Risc-V_SoC) project this
program was written for.

## How to build

This program needs the full `Femto_Risc-V_SoC` project (its `CMakeLists.txt`,
`hw/` RTL and toolchain) to assemble, synthesize and flash to the board. These
files are the software side only, kept here as the course's record of the first
assembly program.
