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