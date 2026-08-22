#ifndef NF_LOADER_ELF_LOADER_HPP_
#define NF_LOADER_ELF_LOADER_HPP_

#include <cstdint>
#include <expected>
#include <span>
#include <string>

#include "nf/mem/memory.hpp"

namespace nf::loader {

// Reports why `bytes` was not a well-formed, statically-linked,
// little-endian ELF32/RISC-V executable. Nightfury only ever loads
// binaries it built itself (riscv-tests, hand-assembled fixtures), so
// this parser is intentionally narrow rather than a general-purpose ELF
// reader -- see elf_loader.cpp for the exact ET_EXEC/EM_RISCV assumptions.
//
// A malformed input is an ordinary, expected outcome (a caller passed a
// bad path or a non-RISC-V binary), so it's reported through the return
// value rather than an exception -- see mem::AccessFault for the
// contrasting case of an error that represents a simulator/test bug.
struct ElfFormatError {
    std::string message;
};

struct ElfImage {
    uint32_t entry_pc;  // e_entry -- where the interpreter's pc_ should start
};

// Parses `bytes` as an ELF32/RISC-V/ET_EXEC image and writes every
// PT_LOAD segment's file contents into `memory` at its p_vaddr via
// Memory::WriteBlob. `memory` must already be sized to cover the image's
// address range -- this function does not resize or relocate anything.
std::expected<ElfImage, ElfFormatError> LoadElf(std::span<const uint8_t> bytes,
                                                 mem::Memory &memory);

// Reads the whole file at `path` into memory and forwards to LoadElf.
std::expected<ElfImage, ElfFormatError> LoadElfFile(const std::string &path,
                                                     mem::Memory &memory);

}  // namespace nf::loader

#endif  // NF_LOADER_ELF_LOADER_HPP_
