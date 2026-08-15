#! /usr/bin/bash
# 1. A trivial bare-metal RV32IM program: set a0=42, then ecall the
#    riscv-tests/HTIF pass convention (a7=93 is the "exit" syscall number)
cat > tiny.S << 'EOF'
.section .text
.globl _start
_start:
    li a0, 42
    li a7, 93
    ecall
EOF

# 2. Compile + link as a static, bare-metal RV32IM executable.
#    -Wl,-n (NMAGIC) disables the linker's default page-alignment padding
#    on PT_LOAD segments -- without it, the segment's vaddr gets backed up
#    a full page below -Ttext
riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 \
    -nostdlib -nostartfiles -Wl,-n -Ttext=0x80000000 \
    -o tiny.elf tiny.S

# 3. Inspect the ELF header fields your parser reads: class, endianness,
#    type, machine, entry point.
riscv64-unknown-elf-readelf -h tiny.elf

# 4. Inspect the program headers -- confirm there's exactly one PT_LOAD
#    segment and that VirtAddr == 0x80000000 (no page-rounding).
riscv64-unknown-elf-readelf -l tiny.elf

# 5. Disassemble to confirm the encoded bytes match what you expect
#    ProcessInstruction to decode (li expands to addi; ecall is its own
#    32-bit word).
riscv64-unknown-elf-objdump -d tiny.elf > tiny.elf.dis

# 6. Raw bytes at the file offset the PT_LOAD segment points at, if you
#    want to eyeball exactly what LoadElf's WriteBlob will copy.
riscv64-unknown-elf-objcopy -O binary tiny.elf tiny.bin
xxd tiny.bin

