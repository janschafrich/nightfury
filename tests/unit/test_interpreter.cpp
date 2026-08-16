#include <catch2/catch_test_macros.hpp>

#include "nf/core/interpreter.hpp"
#include "nf/mem/memory.hpp"

using nf::core::Interpreter;
using nf::mem::ElementSize;
using nf::mem::Memory;

// These tests observe the interpreter purely through memory side effects
// (sw to a scratch address) rather than adding test-only accessors to
// Interpreter for pc_/regfile_/csrs_ -- the same black-box discipline the
// interpreter itself uses to signal riscv-tests results via tohost.
namespace {

constexpr uint32_t kOpImm = 0b0010011;
constexpr uint32_t kOpStore = 0b0100011;
constexpr uint32_t kOpSystem = 0b1110011;

constexpr uint32_t kCsrMtvec = 0x305;
constexpr uint32_t kCsrMepc = 0x341;
constexpr uint32_t kCsrMcause = 0x342;

uint32_t encode_i(uint32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd,
                   uint32_t opcode) {
    return (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

uint32_t encode_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3,
                   uint32_t opcode) {
    const uint32_t u = static_cast<uint32_t>(imm);
    return ((u >> 5 & 0x7F) << 25) | (rs2 << 20) | (rs1 << 15) |
           (funct3 << 12) | ((u & 0x1F) << 7) | opcode;
}

}  // namespace

TEST_CASE("Interpreter traps ecall to mtvec and records mepc/mcause", "[interpreter]") {
    Memory memory(0, 0x100);
    Interpreter interp(memory, 0);

    // 0:  csrrwi x0, mtvec, 24      -- mtvec = 24 (uimm is only 5 bits, max 31)
    memory.Store(ElementSize::kWord, 0, encode_i(kCsrMtvec, 24, 0b101, 0, kOpSystem));
    // 4:  ecall                     -- mepc <- 4 (pc of this ecall), mcause <- 0xb, pc <- mtvec
    memory.Store(ElementSize::kWord, 4, encode_i(0, 0, 0b000, 0, kOpSystem));
    // 24: csrrs x1, mepc, x0        -- x1 = mepc
    memory.Store(ElementSize::kWord, 24, encode_i(kCsrMepc, 0, 0b010, 1, kOpSystem));
    // 28: csrrs x2, mcause, x0      -- x2 = mcause
    memory.Store(ElementSize::kWord, 28, encode_i(kCsrMcause, 0, 0b010, 2, kOpSystem));
    // 32: sw x1, 64(x0)
    memory.Store(ElementSize::kWord, 32, encode_s(64, 1, 0, 0b010, kOpStore));
    // 36: sw x2, 68(x0)
    memory.Store(ElementSize::kWord, 36, encode_s(68, 2, 0, 0b010, kOpStore));

    for (int i = 0; i < 6; ++i) interp.ProcessInstruction();

    CHECK(memory.Load(ElementSize::kWord, 64) == 4u);     // mepc
    CHECK(memory.Load(ElementSize::kWord, 68) == 0x0Bu);  // mcause: CAUSE_MACHINE_ECALL
}

TEST_CASE("Interpreter's mret jumps pc to mepc", "[interpreter]") {
    Memory memory(0, 0x100);
    Interpreter interp(memory, 0);

    // 0:  csrrwi x0, mepc, 12       -- mepc = 12
    memory.Store(ElementSize::kWord, 0, encode_i(kCsrMepc, 12, 0b101, 0, kOpSystem));
    // 4:  mret                      -- pc <- mepc (0x302 = mret's fixed imm12)
    memory.Store(ElementSize::kWord, 4, encode_i(0x302, 0, 0b000, 0, kOpSystem));
    // 12: addi x5, x0, 77           -- only reached if mret actually jumped here
    memory.Store(ElementSize::kWord, 12, encode_i(77, 0, 0b000, 5, kOpImm));
    // 16: sw x5, 64(x0)
    memory.Store(ElementSize::kWord, 16, encode_s(64, 5, 0, 0b010, kOpStore));

    for (int i = 0; i < 4; ++i) interp.ProcessInstruction();

    CHECK(memory.Load(ElementSize::kWord, 64) == 77u);
}

TEST_CASE("csrrw/csrrwi write the new value and return the old value in rd", "[interpreter]") {
    Memory memory(0, 0x100);
    Interpreter interp(memory, 0);

    // 0: csrrwi x0, mtvec, 20   -- mtvec = 20, old value (0) discarded into x0
    memory.Store(ElementSize::kWord, 0, encode_i(kCsrMtvec, 20, 0b101, 0, kOpSystem));
    // 4: csrrw x1, mtvec, x0    -- x1 = old mtvec (20); mtvec = x0's value (0)
    memory.Store(ElementSize::kWord, 4, encode_i(kCsrMtvec, 0, 0b001, 1, kOpSystem));
    // 8: csrrs x2, mtvec, x0    -- x2 = mtvec readback (0), x0 as rs1 means no-op OR
    memory.Store(ElementSize::kWord, 8, encode_i(kCsrMtvec, 0, 0b010, 2, kOpSystem));
    // 12: sw x1, 64(x0)   -- expect 20 (csrrw returned the pre-write value)
    memory.Store(ElementSize::kWord, 12, encode_s(64, 1, 0, 0b010, kOpStore));
    // 16: sw x2, 68(x0)   -- expect 0 (mtvec really got overwritten)
    memory.Store(ElementSize::kWord, 16, encode_s(68, 2, 0, 0b010, kOpStore));

    for (int i = 0; i < 5; ++i) interp.ProcessInstruction();

    CHECK(memory.Load(ElementSize::kWord, 64) == 20u);
    CHECK(memory.Load(ElementSize::kWord, 68) == 0u);
}

TEST_CASE("csrrs/csrrc perform OR/AND-NOT read-modify-write", "[interpreter]") {
    Memory memory(0, 0x100);
    Interpreter interp(memory, 0);

    // 0: addi x2, x0, 0x30       -- x2 = 0b110000
    memory.Store(ElementSize::kWord, 0, encode_i(0x30, 0, 0b000, 2, kOpImm));
    // 4: csrrwi x0, mtvec, 0x0F  -- mtvec = 0b001111
    memory.Store(ElementSize::kWord, 4, encode_i(kCsrMtvec, 0x0F, 0b101, 0, kOpSystem));
    // 8: csrrs x1, mtvec, x2     -- x1 = old (0x0F); mtvec = 0x0F | 0x30 = 0x3F
    memory.Store(ElementSize::kWord, 8, encode_i(kCsrMtvec, 2, 0b010, 1, kOpSystem));
    // 12: csrrc x3, mtvec, x2    -- x3 = old (0x3F); mtvec = 0x3F & ~0x30 = 0x0F
    memory.Store(ElementSize::kWord, 12, encode_i(kCsrMtvec, 2, 0b011, 3, kOpSystem));
    // 16: sw x1, 64(x0)  -- expect 0x0F
    memory.Store(ElementSize::kWord, 16, encode_s(64, 1, 0, 0b010, kOpStore));
    // 20: sw x3, 68(x0)  -- expect 0x3F
    memory.Store(ElementSize::kWord, 20, encode_s(68, 3, 0, 0b010, kOpStore));

    for (int i = 0; i < 6; ++i) interp.ProcessInstruction();

    CHECK(memory.Load(ElementSize::kWord, 64) == 0x0Fu);
    CHECK(memory.Load(ElementSize::kWord, 68) == 0x3Fu);
}
