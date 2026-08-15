#include "nf/loader/elf_loader.hpp"

#include <algorithm>
#include <fstream>
#include <vector>

namespace nf::loader {

namespace {

// -- ELF32 layout constants (elf(5)) --------------------------------------
// On-disk sizes, not sizeof(SomeStruct): we read fields by explicit byte
// offset below rather than overlaying a struct on the buffer, since that
// would be a strict-aliasing violation and would silently depend on the
// host compiler not padding the struct differently from the file layout.
constexpr size_t kEhdrSize = 52;
constexpr size_t kPhdrSize = 32;

constexpr uint8_t kElfClass32 = 1;   // e_ident[EI_CLASS]: 32-bit objects
constexpr uint8_t kElfData2Lsb = 1;  // e_ident[EI_DATA]: little-endian
constexpr uint16_t kEtExec = 2;      // e_type: fully linked executable
constexpr uint16_t kEmRiscv = 243;   // e_machine: RISC-V
constexpr uint32_t kPtLoad = 1;      // p_type: loadable segment

uint16_t ReadU16(std::span<const uint8_t> b, size_t off) {
    return static_cast<uint16_t>(b[off]) | static_cast<uint16_t>(b[off + 1] << 8);
}

uint32_t ReadU32(std::span<const uint8_t> b, size_t off) {
    return static_cast<uint32_t>(b[off]) | (static_cast<uint32_t>(b[off + 1]) << 8) |
           (static_cast<uint32_t>(b[off + 2]) << 16) | (static_cast<uint32_t>(b[off + 3]) << 24);
}

struct ElfHeader {
    uint32_t entry;
    uint32_t phoff;
    uint16_t phentsize;
    uint16_t phnum;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t filesz;
    uint32_t memsz;
};

ElfHeader ParseElf32Header(std::span<const uint8_t> bytes) {
    if (bytes.size() < kEhdrSize) {
        throw ElfFormatError("file too small to contain an ELF header");
    }
    if (bytes[0] != 0x7F || bytes[1] != 'E' || bytes[2] != 'L' || bytes[3] != 'F') {
        throw ElfFormatError("missing 0x7F 'E' 'L' 'F' magic");
    }
    if (bytes[4] != kElfClass32) {
        throw ElfFormatError("not a 32-bit ELF (EI_CLASS != ELFCLASS32)");
    }
    if (bytes[5] != kElfData2Lsb) {
        throw ElfFormatError("not little-endian (EI_DATA != ELFDATA2LSB)");
    }
    if (ReadU16(bytes, 16) != kEtExec) {
        throw ElfFormatError("not a static executable (e_type != ET_EXEC)");
    }
    if (ReadU16(bytes, 18) != kEmRiscv) {
        throw ElfFormatError("e_machine != EM_RISCV");
    }

    ElfHeader hdr;
    hdr.entry = ReadU32(bytes, 24);
    hdr.phoff = ReadU32(bytes, 28);
    hdr.phentsize = ReadU16(bytes, 42);
    hdr.phnum = ReadU16(bytes, 44);
    return hdr;
}

// Elf32_Phdr field order is p_type, p_offset, p_vaddr, p_paddr, p_filesz,
// p_memsz, p_flags, p_align. Note this differs from Elf64_Phdr, which
// moves p_flags right after p_type to keep the 64-bit fields aligned --
// a classic bug if this parser is ever "generalized" by widening ints
// without rechecking field order.
ProgramHeader ParseProgramHeader(std::span<const uint8_t> bytes, size_t off) {
    ProgramHeader ph;
    ph.type = ReadU32(bytes, off + 0);
    ph.offset = ReadU32(bytes, off + 4);
    ph.vaddr = ReadU32(bytes, off + 8);
    // p_paddr at off+12 is unused: RV32 has no separate physical address here.
    ph.filesz = ReadU32(bytes, off + 16);
    ph.memsz = ReadU32(bytes, off + 20);
    return ph;
}

}  // namespace

ElfImage LoadElf(std::span<const uint8_t> bytes, mem::Memory &memory) {
    const ElfHeader hdr = ParseElf32Header(bytes);

    for (uint16_t i = 0; i < hdr.phnum; ++i) {
        const size_t off = hdr.phoff + static_cast<size_t>(i) * hdr.phentsize;
        if (off + kPhdrSize > bytes.size()) {
            throw ElfFormatError("program header table runs past end of file");
        }

        const ProgramHeader ph = ParseProgramHeader(bytes, off);
        if (ph.type != kPtLoad || ph.filesz == 0) {
            continue;
        }
        if (ph.offset + ph.filesz > bytes.size()) {
            throw ElfFormatError("PT_LOAD segment runs past end of file");
        }

        // memsz > filesz is .bss. Memory's backing store is zero-initialized
        // at construction (std::vector value-init), so the [filesz, memsz)
        // tail is already zero and needs no explicit write here. That's an
        // invariant of a freshly constructed Memory, not of WriteBlob --
        // reloading a second image into the same Memory would need an
        // explicit zero-fill first.
        memory.WriteBlob(ph.vaddr, bytes.subspan(ph.offset, ph.filesz));
    }

    return ElfImage{hdr.entry};
}

ElfImage LoadElfFile(const std::string &path, mem::Memory &memory) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw ElfFormatError("cannot open file: " + path);
    }

    const std::streamsize size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char *>(bytes.data()), size)) {
        throw ElfFormatError("failed to read file: " + path);
    }

    return LoadElf(bytes, memory);
}

}  // namespace nf::loader
