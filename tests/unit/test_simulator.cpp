#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "nf/sim/simulator.hpp"

using nf::sim::Simulator;

namespace {

constexpr size_t kEhdrSize = 52;
constexpr size_t kPhdrSize = 32;
constexpr size_t kShdrSize = 40;
constexpr size_t kSymSize = 16;

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

uint32_t EncodeU(uint32_t imm20, uint32_t rd, uint32_t opcode) {
    return (imm20 << 12) | (rd << 7) | opcode;
}

uint32_t EncodeI(uint32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t opcode) {
    return (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

uint32_t EncodeS(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t opcode) {
    const uint32_t u = static_cast<uint32_t>(imm);
    return ((u >> 5 & 0x7F) << 25) | (rs2 << 20) | (rs1 << 15) |
           (funct3 << 12) | ((u & 0x1F) << 7) | opcode;
}

// Builds a page-aligned tohost store: `lui x1, tohost>>12; addi x2, x0,
// tohost_value; sw x2, 0(x1)`. Requires tohost's low 12 bits to be zero,
// which every real riscv-tests linker script satisfies.
std::vector<uint8_t> BuildTohostStoreProgram(uint32_t tohost_addr, uint32_t tohost_value) {
    REQUIRE((tohost_addr & 0xFFF) == 0);
    std::vector<uint8_t> code(12);
    PutU32(code, 0, EncodeU(tohost_addr >> 12, /*rd=*/1, /*opcode=*/0b0110111));       // lui x1, ...
    PutU32(code, 4, EncodeI(tohost_value, /*rs1=*/0, /*funct3=*/0, /*rd=*/2, 0b0010011)); // addi x2, x0, ...
    PutU32(code, 8, EncodeS(0, /*rs2=*/2, /*rs1=*/1, /*funct3=*/0b010, 0b0100011));    // sw x2, 0(x1)
    return code;
}

// Minimal ELF32/RISC-V/ET_EXEC with one PT_LOAD segment and a symtab/strtab
// naming "tohost" at `tohost_addr` -- same shape as test_elf_loader.cpp's
// builders, kept separate since this is the only test file that needs a
// program with actual architectural effect (a real halt).
std::vector<uint8_t> BuildElfWithTohost(uint32_t vaddr, const std::vector<uint8_t> &payload,
                                        uint32_t tohost_addr) {
    const size_t phoff = kEhdrSize;
    const size_t data_off = phoff + kPhdrSize;

    std::vector<uint8_t> b(data_off + payload.size(), 0);
    b[0] = 0x7F; b[1] = 'E'; b[2] = 'L'; b[3] = 'F';
    b[4] = 1;  // EI_CLASS = ELFCLASS32
    b[5] = 1;  // EI_DATA  = ELFDATA2LSB
    PutU16(b, 16, 2);    // e_type = ET_EXEC
    PutU16(b, 18, 243);  // e_machine = EM_RISCV
    PutU32(b, 24, vaddr);                              // e_entry
    PutU32(b, 28, static_cast<uint32_t>(phoff));        // e_phoff
    PutU16(b, 42, static_cast<uint16_t>(kPhdrSize));    // e_phentsize
    PutU16(b, 44, 1);                                   // e_phnum

    PutU32(b, phoff + 0, 1);                                       // p_type = PT_LOAD
    PutU32(b, phoff + 4, static_cast<uint32_t>(data_off));         // p_offset
    PutU32(b, phoff + 8, vaddr);                                   // p_vaddr
    PutU32(b, phoff + 16, static_cast<uint32_t>(payload.size()));  // p_filesz
    PutU32(b, phoff + 20, static_cast<uint32_t>(payload.size()));  // p_memsz

    std::copy(payload.begin(), payload.end(), b.begin() + static_cast<std::ptrdiff_t>(data_off));

    // .strtab: index 0 is the reserved empty name, "tohost" right after.
    std::vector<uint8_t> strtab = {0};
    const size_t name_off = strtab.size();
    strtab.insert(strtab.end(), {'t', 'o', 'h', 'o', 's', 't', 0});

    // .symtab: index 0 is the reserved null symbol, index 1 is "tohost".
    std::vector<uint8_t> symtab(kSymSize, 0);
    std::vector<uint8_t> sym(kSymSize, 0);
    PutU32(sym, 0, static_cast<uint32_t>(name_off));  // st_name
    PutU32(sym, 4, tohost_addr);                      // st_value
    symtab.insert(symtab.end(), sym.begin(), sym.end());

    const size_t symtab_off = b.size();
    b.insert(b.end(), symtab.begin(), symtab.end());
    const size_t strtab_off = b.size();
    b.insert(b.end(), strtab.begin(), strtab.end());

    const size_t shoff = b.size();
    b.resize(shoff + 3 * kShdrSize, 0);
    PutU32(b, shoff + kShdrSize + 4, 2);                                      // [1].sh_type = SHT_SYMTAB
    PutU32(b, shoff + kShdrSize + 16, static_cast<uint32_t>(symtab_off));     // [1].sh_offset
    PutU32(b, shoff + kShdrSize + 20, static_cast<uint32_t>(symtab.size())); // [1].sh_size
    PutU32(b, shoff + kShdrSize + 24, 2);                                     // [1].sh_link -> section 2
    PutU32(b, shoff + kShdrSize + 36, static_cast<uint32_t>(kSymSize));       // [1].sh_entsize
    PutU32(b, shoff + 2 * kShdrSize + 4, 3);                                     // [2].sh_type = SHT_STRTAB
    PutU32(b, shoff + 2 * kShdrSize + 16, static_cast<uint32_t>(strtab_off));    // [2].sh_offset
    PutU32(b, shoff + 2 * kShdrSize + 20, static_cast<uint32_t>(strtab.size())); // [2].sh_size
    PutU32(b, 32, static_cast<uint32_t>(shoff));      // e_shoff
    PutU16(b, 46, static_cast<uint16_t>(kShdrSize));  // e_shentsize
    PutU16(b, 48, 3);                                 // e_shnum

    return b;
}

// Writes `bytes` to a fresh temp file and returns its path; the file
// outlives the object so Simulator's constructor can open it by path.
std::string WriteTempElf(const std::vector<uint8_t> &bytes, const char *name) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    file.close();
    return path.string();
}

}  // namespace

TEST_CASE("Simulator runs a program to a passing tohost store", "[simulator]") {
    constexpr uint32_t kBase = 0x80000000u;
    constexpr uint32_t kTohost = 0x80001000u;

    const auto payload = BuildTohostStoreProgram(kTohost, /*tohost_value=*/1);
    const auto bytes = BuildElfWithTohost(kBase, payload, kTohost);
    const auto path = WriteTempElf(bytes, "nf_test_simulator_pass.elf");

    Simulator sim(path, kBase, 0x2000);
    const auto result = sim.Run();

    CHECK(result.passed);
    CHECK(result.tohost_value == 1u);
}

TEST_CASE("Simulator reports failure and the failing test number from tohost", "[simulator]") {
    constexpr uint32_t kBase = 0x80000000u;
    constexpr uint32_t kTohost = 0x80001000u;
    constexpr uint32_t kFailingTestNumber = 3u;

    const auto payload =
        BuildTohostStoreProgram(kTohost, /*tohost_value=*/(kFailingTestNumber << 1) | 1);
    const auto bytes = BuildElfWithTohost(kBase, payload, kTohost);
    const auto path = WriteTempElf(bytes, "nf_test_simulator_fail.elf");

    Simulator sim(path, kBase, 0x2000);
    const auto result = sim.Run();

    CHECK_FALSE(result.passed);
    CHECK((result.tohost_value >> 1) == kFailingTestNumber);
}

TEST_CASE("Simulator's constructor throws for a binary with no tohost symbol", "[simulator]") {
    constexpr uint32_t kBase = 0x80000000u;
    const std::vector<uint8_t> payload = {0x13, 0x00, 0x00, 0x00};  // addi x0,x0,0 (nop)

    // No AppendSymbolTable equivalent here -- just the plain PT_LOAD image,
    // same shape test_elf_loader.cpp's BuildMinimalElf produces.
    const size_t phoff = kEhdrSize;
    const size_t data_off = phoff + kPhdrSize;
    std::vector<uint8_t> b(data_off + payload.size(), 0);
    b[0] = 0x7F; b[1] = 'E'; b[2] = 'L'; b[3] = 'F';
    b[4] = 1; b[5] = 1;
    PutU16(b, 16, 2);
    PutU16(b, 18, 243);
    PutU32(b, 24, kBase);
    PutU32(b, 28, static_cast<uint32_t>(phoff));
    PutU16(b, 42, static_cast<uint16_t>(kPhdrSize));
    PutU16(b, 44, 1);
    PutU32(b, phoff + 0, 1);
    PutU32(b, phoff + 4, static_cast<uint32_t>(data_off));
    PutU32(b, phoff + 8, kBase);
    PutU32(b, phoff + 16, static_cast<uint32_t>(payload.size()));
    PutU32(b, phoff + 20, static_cast<uint32_t>(payload.size()));
    std::copy(payload.begin(), payload.end(), b.begin() + static_cast<std::ptrdiff_t>(data_off));

    const auto path = WriteTempElf(b, "nf_test_simulator_no_tohost.elf");

    CHECK_THROWS_AS(Simulator(path, kBase, 0x1000), std::runtime_error);
}
