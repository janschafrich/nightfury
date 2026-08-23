#include "nf/loader/elf_loader.hpp"

#include <expected>
#include <optional>
#include <string_view>
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
constexpr size_t kShdrSize = 40;

constexpr uint8_t kElfClass32 = 1;   // e_ident[EI_CLASS]: 32-bit objects
constexpr uint8_t kElfData2Lsb = 1;  // e_ident[EI_DATA]: little-endian
constexpr uint16_t kEtExec = 2;      // e_type: fully linked executable
constexpr uint16_t kEmRiscv = 243;   // e_machine: RISC-V
constexpr uint32_t kPtLoad = 1;      // p_type: loadable segment
constexpr uint32_t kShtSymtab = 2;   // sh_type: symbol table

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
    uint32_t shoff;
    uint16_t shentsize;
    uint16_t shnum;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t filesz;
    uint32_t memsz;
};

// Fields needed to walk a symbol table: for SHT_SYMTAB, sh_link is the
// section index of the associated SHT_STRTAB and sh_entsize is the size of
// one Elf32_Sym (16, but read rather than assumed -- same rationale as
// reading e_phentsize instead of hardcoding it).
struct SectionHeader {
    uint32_t type;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t entsize;
};

std::expected<ElfHeader, ElfFormatError> ParseElf32Header(std::span<const uint8_t> bytes) {
    if (bytes.size() < kEhdrSize) {
        return std::unexpected(ElfFormatError{"file too small to contain an ELF header"});
    }
    if (bytes[0] != 0x7F || bytes[1] != 'E' || bytes[2] != 'L' || bytes[3] != 'F') {
        return std::unexpected(ElfFormatError{"missing 0x7F 'E' 'L' 'F' magic"});
    }
    if (bytes[4] != kElfClass32) {
        return std::unexpected(ElfFormatError{"not a 32-bit ELF (EI_CLASS != ELFCLASS32)"});
    }
    if (bytes[5] != kElfData2Lsb) {
        return std::unexpected(ElfFormatError{"not little-endian (EI_DATA != ELFDATA2LSB)"});
    }
    if (ReadU16(bytes, 16) != kEtExec) {
        return std::unexpected(ElfFormatError{"not a static executable (e_type != ET_EXEC)"});
    }
    if (ReadU16(bytes, 18) != kEmRiscv) {
        return std::unexpected(ElfFormatError{"e_machine != EM_RISCV"});
    }

    return ElfHeader{
        .entry = ReadU32(bytes, 24),
        .phoff = ReadU32(bytes, 28),
        .phentsize = ReadU16(bytes, 42),
        .phnum = ReadU16(bytes, 44),
        .shoff = ReadU32(bytes, 32),
        .shentsize = ReadU16(bytes, 46),
        .shnum = ReadU16(bytes, 48),
    };
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

// Elf32_Shdr field order: sh_name, sh_type, sh_flags, sh_addr, sh_offset,
// sh_size, sh_link, sh_info, sh_addralign, sh_entsize.
SectionHeader ParseSectionHeader(std::span<const uint8_t> bytes, size_t off) {
    SectionHeader sh;
    sh.type = ReadU32(bytes, off + 4);
    sh.offset = ReadU32(bytes, off + 16);
    sh.size = ReadU32(bytes, off + 20);
    sh.link = ReadU32(bytes, off + 24);
    sh.entsize = ReadU32(bytes, off + 36);
    return sh;
}

std::string_view ReadCString(std::span<const uint8_t> bytes, size_t off) {
    size_t end = off;
    while (end < bytes.size() && bytes[end] != 0) {
        ++end;
    }
    return std::string_view(reinterpret_cast<const char *>(bytes.data() + off), end - off);
}

// Resolves `name`'s address via the first SHT_SYMTAB section and its
// SHT_STRTAB (sh_link). Absence of a match -- no section headers, no
// symtab, or no symbol by that name -- is reported as nullopt rather than
// ElfFormatError: a well-formed ELF with no such symbol isn't malformed,
// it just isn't a riscv-tests binary. A malformed section header table
// (truncated, out-of-range sh_link) is likewise folded into nullopt here
// rather than surfaced as a load error, since it only affects this optional
// lookup -- PT_LOAD segments (the part LoadElf must get right) don't
// depend on section headers at all.
std::optional<uint32_t> FindSymbolAddress(std::span<const uint8_t> bytes, const ElfHeader &hdr,
                                           std::string_view name) {
    for (uint16_t i = 0; i < hdr.shnum; ++i) {
        const size_t off = static_cast<size_t>(hdr.shoff) + static_cast<size_t>(i) * hdr.shentsize;
        if (off + kShdrSize > bytes.size()) {
            return std::nullopt;
        }
        const SectionHeader sh = ParseSectionHeader(bytes, off);
        if (sh.type != kShtSymtab) {
            continue;
        }
        if (sh.link >= hdr.shnum || sh.entsize == 0) {
            return std::nullopt;
        }
        const size_t strtab_off =
            static_cast<size_t>(hdr.shoff) + static_cast<size_t>(sh.link) * hdr.shentsize;
        const SectionHeader strtab = ParseSectionHeader(bytes, strtab_off);

        const uint32_t sym_count = sh.size / sh.entsize;
        for (uint32_t s = 0; s < sym_count; ++s) {
            const size_t sym_off = sh.offset + static_cast<size_t>(s) * sh.entsize;
            if (sym_off + 8 > bytes.size()) {
                return std::nullopt;
            }
            const uint32_t st_name = ReadU32(bytes, sym_off + 0);
            if (st_name == 0) {
                continue;
            }
            if (ReadCString(bytes, strtab.offset + st_name) == name) {
                return ReadU32(bytes, sym_off + 4);  // st_value
            }
        }
        return std::nullopt;  // found the symtab; it just has no "tohost"
    }
    return std::nullopt;  // no SHT_SYMTAB section at all
}

}  // namespace

std::expected<ElfImage, ElfFormatError> LoadElf(std::span<const uint8_t> bytes,
                                                 mem::Memory &memory) {
    // Discards elf header and program header and writes .text and .data to memory
    // The .bss is not explicitly initialized to zero here, since this is done by
    // construction, when instantiating the memory.
    const auto hdr = ParseElf32Header(bytes);
    if (!hdr) {
        return std::unexpected(hdr.error());
    }

    for (uint16_t i = 0; i < hdr->phnum; ++i) {
        const size_t off = hdr->phoff + static_cast<size_t>(i) * hdr->phentsize;
        if (off + kPhdrSize > bytes.size()) {
            return std::unexpected(ElfFormatError{"program header table runs past end of file"});
        }

        const ProgramHeader ph = ParseProgramHeader(bytes, off);
        if (ph.type != kPtLoad || ph.filesz == 0) {
            continue;
        }
        if (ph.offset + ph.filesz > bytes.size()) {
            return std::unexpected(ElfFormatError{"PT_LOAD segment runs past end of file"});
        }

        // memsz > filesz is .bss. Memory's backing store is zero-initialized
        // at construction (std::vector value-init), so the [filesz, memsz)
        // tail is already zero and needs no explicit write here. That's an
        // invariant of a freshly constructed Memory, not of WriteBlob --
        // reloading a second image into the same Memory would need an
        // explicit zero-fill first.
        // Only read to filesz (and not memsz), as zeros of the .bss segment
        // are not included in the binary to save disk space.
        memory.WriteBlob(ph.vaddr, bytes.subspan(ph.offset, ph.filesz));
    }

    return ElfImage{.entry_pc = hdr->entry, .tohost_addr = FindSymbolAddress(bytes, *hdr, "tohost")};
}

std::expected<ElfImage, ElfFormatError> LoadElfFile(const std::string &path,
                                                     mem::Memory &memory) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return std::unexpected(ElfFormatError{"cannot open file: " + path});
    }

    const std::streamsize size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char *>(bytes.data()), size)) {
        return std::unexpected(ElfFormatError{"failed to read file: " + path});
    }

    return LoadElf(bytes, memory);
}

}  // namespace nf::loader
