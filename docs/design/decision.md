# Register File

- use class to model state
- use read and write methods to ease adding timing information later
- skip out of bounds access handling since the decoder guarantess that rs1, rs2, rd will always be in the range [0:31].


# ALU

- implement as pure function to avoid any state
- treat it like combinational logic
- convert from ArchOps to AluOps to take advantage off similarity of immediate and non-immediate operations (ADDI, ADD)


# Memory

- keep memory dumb: it just moves bytes, independent of ISA
- sign extension is handled at caller (interpreter)


# Decoder

- Eager decode: to prevent decode and pipeline logic from drifting apart, everything needed laster is derived at decode
- Learning: this apparently how Gem5 does it, in contrast to hardware
- Hardware goes for lazy decoding to save on area (wiring)