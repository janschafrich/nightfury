Uses riscvtests located here:

Build tests using:
```sh
cd riscv-tests/isa
make XLEN=32 rv32ui-p-add
riscv64-unknown-elf-nm rv32ui-p-add | grep -E "tohost|fromhost|begin_signature"
```