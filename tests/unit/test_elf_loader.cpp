#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "nf/loader/elf_loader.hpp"
#include "nf/mem/memory.hpp"

using nf::loader::ElfFormatError;
using nf::loader::LoadElf;
using nf::mem::ElementSize;
using nf::mem::Memory;

namespace {

constexpr size_t kEhdrSize = 52;
constexpr size_t kPhdrSize = 32;

void PutU16(std::vector<uint8_t> &b, size_t off, uint16_t v) {
    b[off] = static_cast<uint8_t>(v);
    b[off + 1] = static_cast<uint8_t>(v >> 8);
}

void PutU32(std::vector<uint8_t> &b, size_t off, uint32_t v) {
    b[off] = static_cast<uint8_t>(v);
    b[off + 1] = static_cast<uint8_t>(v >> 8);
    b[off + 2] = static_cast<uint8_t>(v >> 16);
    b[off + 3] = static_cast<uint8_t>(v >> 24);
}

// Builds a minimal but well-formed ELF32/RISC-V/ET_EXEC image with a
// single PT_LOAD segment containing `payload`, loaded at `vaddr`.
std::vector<uint8_t> BuildMinimalElf(uint32_t entry, uint32_t vaddr,
                                      const std::vector<uint8_t> &payload) {
    const size_t phoff = kEhdrSize;
    const size_t data_off = phoff + kPhdrSize;

    std::vector<uint8_t> b(data_off + payload.size(), 0);

    b[0] = 0x7F; b[1] = 'E'; b[2] = 'L'; b[3] = 'F';
    b[4] = 1;  // EI_CLASS = ELFCLASS32
    b[5] = 1;  // EI_DATA  = ELFDATA2LSB

    PutU16(b, 16, 2);    // e_type = ET_EXEC
    PutU16(b, 18, 243);  // e_machine = EM_RISCV
    PutU32(b, 24, entry);
    PutU32(b, 28, static_cast<uint32_t>(phoff));      // e_phoff
    PutU16(b, 42, static_cast<uint16_t>(kPhdrSize));  // e_phentsize
    PutU16(b, 44, 1);                                 // e_phnum

    PutU32(b, phoff + 0, 1);                                     // p_type = PT_LOAD
    PutU32(b, phoff + 4, static_cast<uint32_t>(data_off));       // p_offset
    PutU32(b, phoff + 8, vaddr);                                 // p_vaddr
    PutU32(b, phoff + 16, static_cast<uint32_t>(payload.size())); // p_filesz
    PutU32(b, phoff + 20, static_cast<uint32_t>(payload.size())); // p_memsz

    std::copy(payload.begin(), payload.end(), b.begin() + static_cast<std::ptrdiff_t>(data_off));
    return b;
}

// Appends a null section (index 0), an SHT_SYMTAB section (index 1) and its
// SHT_STRTAB (index 2) holding `symbols`, then points e_shoff/e_shentsize/
// e_shnum at them -- everything a real linker's section header table would
// carry except the fields LoadElf's symbol lookup doesn't read.
void AppendSymbolTable(std::vector<uint8_t> &b,
                        const std::vector<std::pair<std::string, uint32_t>> &symbols) {
    constexpr size_t kShdrSize = 40;
    constexpr size_t kSymSize = 16;

    std::vector<uint8_t> strtab = {0};  // index 0 is the reserved empty name
    std::vector<size_t> name_off;
    for (const auto &[name, value] : symbols) {
        name_off.push_back(strtab.size());
        strtab.insert(strtab.end(), name.begin(), name.end());
        strtab.push_back(0);
    }

    std::vector<uint8_t> symtab(kSymSize, 0);  // index 0 is the reserved null symbol
    for (size_t i = 0; i < symbols.size(); ++i) {
        std::vector<uint8_t> entry(kSymSize, 0);
        PutU32(entry, 0, static_cast<uint32_t>(name_off[i]));  // st_name
        PutU32(entry, 4, symbols[i].second);                   // st_value
        symtab.insert(symtab.end(), entry.begin(), entry.end());
    }

    const size_t symtab_off = b.size();
    b.insert(b.end(), symtab.begin(), symtab.end());
    const size_t strtab_off = b.size();
    b.insert(b.end(), strtab.begin(), strtab.end());

    const size_t shoff = b.size();
    b.resize(shoff + 3 * kShdrSize, 0);

    PutU32(b, shoff + kShdrSize + 4, 2);                                       // [1].sh_type = SHT_SYMTAB
    PutU32(b, shoff + kShdrSize + 16, static_cast<uint32_t>(symtab_off));      // [1].sh_offset
    PutU32(b, shoff + kShdrSize + 20, static_cast<uint32_t>(symtab.size()));   // [1].sh_size
    PutU32(b, shoff + kShdrSize + 24, 2);                                      // [1].sh_link -> section 2
    PutU32(b, shoff + kShdrSize + 36, static_cast<uint32_t>(kSymSize));        // [1].sh_entsize

    PutU32(b, shoff + 2 * kShdrSize + 4, 3);                                      // [2].sh_type = SHT_STRTAB
    PutU32(b, shoff + 2 * kShdrSize + 16, static_cast<uint32_t>(strtab_off));     // [2].sh_offset
    PutU32(b, shoff + 2 * kShdrSize + 20, static_cast<uint32_t>(strtab.size()));  // [2].sh_size

    PutU32(b, 32, static_cast<uint32_t>(shoff));      // e_shoff
    PutU16(b, 46, static_cast<uint16_t>(kShdrSize));  // e_shentsize
    PutU16(b, 48, 3);                                 // e_shnum
}

}  // namespace

TEST_CASE("LoadElf copies PT_LOAD contents to the right address and reports entry", "[loader]") {
    constexpr uint32_t kBase = 0x80000000u;
    const std::vector<uint8_t> payload = {0x13, 0x00, 0x00, 0x00};  // addi x0,x0,0 (nop)
    const auto bytes = BuildMinimalElf(/*entry=*/kBase + 4, kBase, payload);

    Memory memory(kBase, 0x1000);
    const auto image = LoadElf(bytes, memory);

    REQUIRE(image.has_value());
    CHECK(image->entry_pc == kBase + 4);
    CHECK(memory.Load(ElementSize::kWord, kBase) == 0x00000013u);
}

TEST_CASE("LoadElf zero-fills bss beyond filesz", "[loader]") {
    constexpr uint32_t kBase = 0x80000000u;
    auto bytes = BuildMinimalElf(kBase, kBase, {0xEF, 0xBE, 0xAD, 0xDE});
    PutU32(bytes, kEhdrSize + 20, 8);  // p_memsz = 8, p_filesz stays 4

    Memory memory(kBase, 0x1000);
    REQUIRE(LoadElf(bytes, memory).has_value());

    CHECK(memory.Load(ElementSize::kWord, kBase) == 0xDEADBEEFu);
    CHECK(memory.Load(ElementSize::kWord, kBase + 4) == 0u);
}

TEST_CASE("LoadElf rejects bad magic", "[loader]") {
    auto bytes = BuildMinimalElf(0x80000000u, 0x80000000u, {0x13, 0, 0, 0});
    bytes[0] = 0x00;

    Memory memory(0x80000000u, 0x1000);
    CHECK_FALSE(LoadElf(bytes, memory).has_value());
}

TEST_CASE("LoadElf rejects non-RISC-V machine", "[loader]") {
    auto bytes = BuildMinimalElf(0x80000000u, 0x80000000u, {0x13, 0, 0, 0});
    PutU16(bytes, 18, 0x3E);  // EM_X86_64

    Memory memory(0x80000000u, 0x1000);
    CHECK_FALSE(LoadElf(bytes, memory).has_value());
}

TEST_CASE("LoadElf rejects a PT_LOAD segment outside the mapped Memory range", "[loader]") {
    constexpr uint32_t kBase = 0x80000000u;
    const auto bytes = BuildMinimalElf(kBase, kBase, {0x13, 0, 0, 0});

    Memory memory(kBase, 2);  // too small for a 4-byte payload
    CHECK_THROWS_AS(LoadElf(bytes, memory), nf::mem::AccessFault);
}

TEST_CASE("LoadElf resolves tohost's address from the symbol table", "[loader]") {
    constexpr uint32_t kBase = 0x80000000u;
    auto bytes = BuildMinimalElf(kBase, kBase, {0x13, 0, 0, 0});
    AppendSymbolTable(bytes, {{"begin_signature", 0x80002000u}, {"tohost", 0x80001000u}});

    Memory memory(kBase, 0x3000);
    const auto image = LoadElf(bytes, memory);

    REQUIRE(image.has_value());
    REQUIRE(image->tohost_addr.has_value());
    CHECK(*image->tohost_addr == 0x80001000u);
}

TEST_CASE("LoadElf reports no tohost for a binary that doesn't define it", "[loader]") {
    constexpr uint32_t kBase = 0x80000000u;
    auto bytes = BuildMinimalElf(kBase, kBase, {0x13, 0, 0, 0});
    AppendSymbolTable(bytes, {{"begin_signature", 0x80002000u}});

    Memory memory(kBase, 0x3000);
    const auto image = LoadElf(bytes, memory);

    REQUIRE(image.has_value());
    CHECK_FALSE(image->tohost_addr.has_value());
}

TEST_CASE("LoadElf reports no tohost when the image has no section headers", "[loader]") {
    constexpr uint32_t kBase = 0x80000000u;
    const auto bytes = BuildMinimalElf(kBase, kBase, {0x13, 0, 0, 0});

    Memory memory(kBase, 0x1000);
    const auto image = LoadElf(bytes, memory);

    REQUIRE(image.has_value());
    CHECK_FALSE(image->tohost_addr.has_value());
}
