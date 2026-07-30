#include "nf/isa/decoder.hpp"

namespace nf::isa {

namespace {

// RV32 base opcodes (word[6:0]).
constexpr uint32_t kOpLoad = 0b0000011;
constexpr uint32_t kOpFence = 0b0001111;
constexpr uint32_t kOpImm = 0b0010011;
constexpr uint32_t kOpAuipc = 0b0010111;
constexpr uint32_t kOpStore = 0b0100011;
constexpr uint32_t kOpReg = 0b0110011;
constexpr uint32_t kOpLui = 0b0110111;
constexpr uint32_t kOpBranch = 0b1100011;
constexpr uint32_t kOpJalr = 0b1100111;
constexpr uint32_t kOpJal = 0b1101111;
constexpr uint32_t kOpSystem = 0b1110011;

constexpr uint32_t kFunct7Base = 0b0000000;
constexpr uint32_t kFunct7Alt = 0b0100000;   // SUB / SRA
constexpr uint32_t kFunct7Mul = 0b0000001;   // RV32M

// Extracts bits [hi:lo] (inclusive) of word, right-justified.
constexpr uint32_t bits(uint32_t word, int hi, int lo) {
    return (word >> lo) & ((1u << (hi - lo + 1)) - 1);
}

// Sign-extends the low `width` bits of value to a full 32-bit signed value.
constexpr int32_t sign_extend(uint32_t value, int width) {
    const uint32_t shift = 32 - static_cast<uint32_t>(width);
    return static_cast<int32_t>(value << shift) >> shift;
}

int32_t imm_i(uint32_t w) { return sign_extend(bits(w, 31, 20), 12); }

int32_t imm_s(uint32_t w) {
    return sign_extend((bits(w, 31, 25) << 5) | bits(w, 11, 7), 12);
}

int32_t imm_b(uint32_t w) {
    const uint32_t v = (bits(w, 31, 31) << 12) | (bits(w, 7, 7) << 11) |
                        (bits(w, 30, 25) << 5) | (bits(w, 11, 8) << 1);
    return sign_extend(v, 13);
}

int32_t imm_u(uint32_t w) { return static_cast<int32_t>(w & 0xFFFFF000u); }

int32_t imm_j(uint32_t w) {
    const uint32_t v = (bits(w, 31, 31) << 20) | (bits(w, 19, 12) << 12) |
                        (bits(w, 20, 20) << 11) | (bits(w, 30, 21) << 1);
    return sign_extend(v, 21);
}

}  // namespace

std::string_view to_string(Mnemonic m) {
    switch (m) {
        case Mnemonic::LUI: return "lui";
        case Mnemonic::AUIPC: return "auipc";
        case Mnemonic::JAL: return "jal";
        case Mnemonic::JALR: return "jalr";
        case Mnemonic::BEQ: return "beq";
        case Mnemonic::BNE: return "bne";
        case Mnemonic::BLT: return "blt";
        case Mnemonic::BGE: return "bge";
        case Mnemonic::BLTU: return "bltu";
        case Mnemonic::BGEU: return "bgeu";
        case Mnemonic::LB: return "lb";
        case Mnemonic::LH: return "lh";
        case Mnemonic::LW: return "lw";
        case Mnemonic::LBU: return "lbu";
        case Mnemonic::LHU: return "lhu";
        case Mnemonic::SB: return "sb";
        case Mnemonic::SH: return "sh";
        case Mnemonic::SW: return "sw";
        case Mnemonic::ADDI: return "addi";
        case Mnemonic::SLTI: return "slti";
        case Mnemonic::SLTIU: return "sltiu";
        case Mnemonic::XORI: return "xori";
        case Mnemonic::ORI: return "ori";
        case Mnemonic::ANDI: return "andi";
        case Mnemonic::SLLI: return "slli";
        case Mnemonic::SRLI: return "srli";
        case Mnemonic::SRAI: return "srai";
        case Mnemonic::ADD: return "add";
        case Mnemonic::SUB: return "sub";
        case Mnemonic::SLL: return "sll";
        case Mnemonic::SLT: return "slt";
        case Mnemonic::SLTU: return "sltu";
        case Mnemonic::XOR: return "xor";
        case Mnemonic::SRL: return "srl";
        case Mnemonic::SRA: return "sra";
        case Mnemonic::OR: return "or";
        case Mnemonic::AND: return "and";
        case Mnemonic::FENCE: return "fence";
        case Mnemonic::ECALL: return "ecall";
        case Mnemonic::EBREAK: return "ebreak";
        case Mnemonic::MUL: return "mul";
        case Mnemonic::MULH: return "mulh";
        case Mnemonic::MULHSU: return "mulhsu";
        case Mnemonic::MULHU: return "mulhu";
        case Mnemonic::DIV: return "div";
        case Mnemonic::DIVU: return "divu";
        case Mnemonic::REM: return "rem";
        case Mnemonic::REMU: return "remu";
        case Mnemonic::INVALID: return "invalid";
    }
    return "invalid";
}

DecodedInstruction Decoder::decode(uint32_t word) {
    DecodedInstruction ins;
    ins.raw = word;

    const uint32_t opcode = bits(word, 6, 0);
    ins.rd = static_cast<uint8_t>(bits(word, 11, 7));
    ins.funct3 = static_cast<uint8_t>(bits(word, 14, 12));
    ins.rs1 = static_cast<uint8_t>(bits(word, 19, 15));
    ins.rs2 = static_cast<uint8_t>(bits(word, 24, 20));
    ins.funct7 = static_cast<uint8_t>(bits(word, 31, 25));

    switch (opcode) {
        case kOpLui:
            ins.format = Format::U;
            ins.mnemonic = Mnemonic::LUI;
            ins.imm = imm_u(word);
            break;

        case kOpAuipc:
            ins.format = Format::U;
            ins.mnemonic = Mnemonic::AUIPC;
            ins.imm = imm_u(word);
            break;

        case kOpJal:
            ins.format = Format::J;
            ins.mnemonic = Mnemonic::JAL;
            ins.imm = imm_j(word);
            break;

        case kOpJalr:
            ins.format = Format::I;
            ins.imm = imm_i(word);
            if (ins.funct3 == 0b000) ins.mnemonic = Mnemonic::JALR;
            break;

        case kOpBranch:
            ins.format = Format::B;
            ins.imm = imm_b(word);
            switch (ins.funct3) {
                case 0b000: ins.mnemonic = Mnemonic::BEQ; break;
                case 0b001: ins.mnemonic = Mnemonic::BNE; break;
                case 0b100: ins.mnemonic = Mnemonic::BLT; break;
                case 0b101: ins.mnemonic = Mnemonic::BGE; break;
                case 0b110: ins.mnemonic = Mnemonic::BLTU; break;
                case 0b111: ins.mnemonic = Mnemonic::BGEU; break;
                default: break;
            }
            break;

        case kOpLoad:
            ins.format = Format::I;
            ins.imm = imm_i(word);
            switch (ins.funct3) {
                case 0b000: ins.mnemonic = Mnemonic::LB; break;
                case 0b001: ins.mnemonic = Mnemonic::LH; break;
                case 0b010: ins.mnemonic = Mnemonic::LW; break;
                case 0b100: ins.mnemonic = Mnemonic::LBU; break;
                case 0b101: ins.mnemonic = Mnemonic::LHU; break;
                default: break;
            }
            break;

        case kOpStore:
            ins.format = Format::S;
            ins.imm = imm_s(word);
            switch (ins.funct3) {
                case 0b000: ins.mnemonic = Mnemonic::SB; break;
                case 0b001: ins.mnemonic = Mnemonic::SH; break;
                case 0b010: ins.mnemonic = Mnemonic::SW; break;
                default: break;
            }
            break;

        case kOpImm:
            ins.format = Format::I;
            ins.imm = imm_i(word);
            switch (ins.funct3) {
                case 0b000: ins.mnemonic = Mnemonic::ADDI; break;
                case 0b010: ins.mnemonic = Mnemonic::SLTI; break;
                case 0b011: ins.mnemonic = Mnemonic::SLTIU; break;
                case 0b100: ins.mnemonic = Mnemonic::XORI; break;
                case 0b110: ins.mnemonic = Mnemonic::ORI; break;
                case 0b111: ins.mnemonic = Mnemonic::ANDI; break;
                case 0b001:
                    if (ins.funct7 == kFunct7Base) ins.mnemonic = Mnemonic::SLLI;
                    break;
                case 0b101:
                    if (ins.funct7 == kFunct7Base) ins.mnemonic = Mnemonic::SRLI;
                    else if (ins.funct7 == kFunct7Alt) ins.mnemonic = Mnemonic::SRAI;
                    break;
                default: break;
            }
            // Shift amount lives in imm[4:0]; SLLI/SRLI/SRAI don't use imm_i's
            // sign extension semantics, but keeping the raw field around too
            // costs nothing and the interpreter can just mask it.
            break;

        case kOpReg:
            ins.format = Format::R;
            switch (ins.funct7) {
                case kFunct7Base:
                    switch (ins.funct3) {
                        case 0b000: ins.mnemonic = Mnemonic::ADD; break;
                        case 0b001: ins.mnemonic = Mnemonic::SLL; break;
                        case 0b010: ins.mnemonic = Mnemonic::SLT; break;
                        case 0b011: ins.mnemonic = Mnemonic::SLTU; break;
                        case 0b100: ins.mnemonic = Mnemonic::XOR; break;
                        case 0b101: ins.mnemonic = Mnemonic::SRL; break;
                        case 0b110: ins.mnemonic = Mnemonic::OR; break;
                        case 0b111: ins.mnemonic = Mnemonic::AND; break;
                        default: break;
                    }
                    break;
                case kFunct7Alt:
                    switch (ins.funct3) {
                        case 0b000: ins.mnemonic = Mnemonic::SUB; break;
                        case 0b101: ins.mnemonic = Mnemonic::SRA; break;
                        default: break;
                    }
                    break;
                case kFunct7Mul:
                    switch (ins.funct3) {
                        case 0b000: ins.mnemonic = Mnemonic::MUL; break;
                        case 0b001: ins.mnemonic = Mnemonic::MULH; break;
                        case 0b010: ins.mnemonic = Mnemonic::MULHSU; break;
                        case 0b011: ins.mnemonic = Mnemonic::MULHU; break;
                        case 0b100: ins.mnemonic = Mnemonic::DIV; break;
                        case 0b101: ins.mnemonic = Mnemonic::DIVU; break;
                        case 0b110: ins.mnemonic = Mnemonic::REM; break;
                        case 0b111: ins.mnemonic = Mnemonic::REMU; break;
                        default: break;
                    }
                    break;
                default: break;
            }
            break;

        case kOpFence:
            ins.format = Format::I;
            if (ins.funct3 == 0b000) ins.mnemonic = Mnemonic::FENCE;
            break;

        case kOpSystem:
            ins.format = Format::I;
            if (ins.funct3 == 0b000) {
                const uint32_t imm12 = bits(word, 31, 20);
                if (imm12 == 0) ins.mnemonic = Mnemonic::ECALL;
                else if (imm12 == 1) ins.mnemonic = Mnemonic::EBREAK;
            }
            break;

        default:
            break;
    }

    return ins;
}

}  // namespace nf::isa
