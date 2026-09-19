Uses riscv-tests located here:


Build tests using:
```sh
cd riscv-tests/isa
make XLEN=32 rv32ui-*
make XLEN=32 rv32um-*
find . type f -iname 'rv32u*v'
find . type f -iname 'rv32u*v' -delete
riscv64-unknown-elf-nm rv32ui-p-add | grep -E "tohost|fromhost|begin_signature"
```

relevant test families for RV32IM (single core, virtual memory disabled)
- base integer: rv32ui-p
- div mul: rv32um-p-

Uses symbols to communciate with the host