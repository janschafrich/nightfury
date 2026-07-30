#pragma once

#include <cstdint>
#include <string_view>

namespace nf::isa {

// RV32I/M instruction encodings. Order matches the six base formats;
// Invalid marks a word that didn't decode to a known instruction.
enum class Format : uint8_t { R, I, S, B, U, J, Invalid };

enum class Mnemonic : uint8_t {
    // RV32I
    LUI, AUIPC, JAL, JALR,
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    LB, LH, LW, LBU, LHU,
    SB, SH, SW,
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    // System
    FENCE, ECALL, EBREAK,
    // RV32M
    MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU,

    INVALID,
};

std::string_view to_string(Mnemonic m);

// Fully identified instruction: which operation, which registers, and the
// sign-extended immediate (format-dependent bit layout already resolved).
// This is the payload the ID pipeline stage will eventually latch into
// the ID/EX register, so decode logic lives here and nowhere else.
struct DecodedInstruction {
    uint32_t raw = 0;
    Format format = Format::Invalid;
    Mnemonic mnemonic = Mnemonic::INVALID;

    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t funct3 = 0;
    uint8_t funct7 = 0;
    int32_t imm = 0;

    bool valid() const { return mnemonic != Mnemonic::INVALID; }
};

class Decoder {
public:
    static DecodedInstruction decode(uint32_t word);
};

}  // namespace nf::isa
