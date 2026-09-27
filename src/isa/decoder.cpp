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
constexpr uint32_t Bits(uint32_t word, int hi, int lo) {
    return (word >> lo) & ((1u << (hi - lo + 1)) - 1);
}

// Sign-extends the low `width` bits of value to a full 32-bit signed value.
constexpr int32_t SignExtend(uint32_t value, int width) {
    const uint32_t shift = 32 - static_cast<uint32_t>(width);
    return static_cast<int32_t>(value << shift) >> shift;
}

int32_t ImmI(uint32_t w) { return SignExtend(Bits(w, 31, 20), 12); }

int32_t ImmS(uint32_t w) {
    return SignExtend((Bits(w, 31, 25) << 5) | Bits(w, 11, 7), 12);
}

int32_t ImmB(uint32_t w) {
    const uint32_t v = (Bits(w, 31, 31) << 12) | (Bits(w, 7, 7) << 11) |
                        (Bits(w, 30, 25) << 5) | (Bits(w, 11, 8) << 1);
    return SignExtend(v, 13);
}

int32_t ImmU(uint32_t w) { return static_cast<int32_t>(w & 0xFFFFF000u); }

int32_t ImmJ(uint32_t w) {
    const uint32_t v = (Bits(w, 31, 31) << 20) | (Bits(w, 19, 12) << 12) |
                        (Bits(w, 20, 20) << 11) | (Bits(w, 30, 21) << 1);
    return SignExtend(v, 21);
}

// CSR address, word[31:20]. Unlike ImmI this is *not* sign-extended -- it's
// an unsigned 12-bit index, not a value, and CSR addresses like mhartid
// (0xF14) have bit 11 set, which ImmI's sign extension would corrupt.
int32_t ImmCsr(uint32_t w) { return static_cast<int32_t>(Bits(w, 31, 20)); }

}  // namespace

std::string_view ToString(Mnemonic m) {
    switch (m) {
        case Mnemonic::kLui: return "lui";
        case Mnemonic::kAuipc: return "auipc";
        case Mnemonic::kJal: return "jal";
        case Mnemonic::kJalr: return "jalr";
        case Mnemonic::kBeq: return "beq";
        case Mnemonic::kBne: return "bne";
        case Mnemonic::kBlt: return "blt";
        case Mnemonic::kBge: return "bge";
        case Mnemonic::kBltu: return "bltu";
        case Mnemonic::kBgeu: return "bgeu";
        case Mnemonic::kLb: return "lb";
        case Mnemonic::kLh: return "lh";
        case Mnemonic::kLw: return "lw";
        case Mnemonic::kLbu: return "lbu";
        case Mnemonic::kLhu: return "lhu";
        case Mnemonic::kSb: return "sb";
        case Mnemonic::kSh: return "sh";
        case Mnemonic::kSw: return "sw";
        case Mnemonic::kAddi: return "addi";
        case Mnemonic::kSlti: return "slti";
        case Mnemonic::kSltiu: return "sltiu";
        case Mnemonic::kXori: return "xori";
        case Mnemonic::kOri: return "ori";
        case Mnemonic::kAndi: return "andi";
        case Mnemonic::kSlli: return "slli";
        case Mnemonic::kSrli: return "srli";
        case Mnemonic::kSrai: return "srai";
        case Mnemonic::kAdd: return "add";
        case Mnemonic::kSub: return "sub";
        case Mnemonic::kSll: return "sll";
        case Mnemonic::kSlt: return "slt";
        case Mnemonic::kSltu: return "sltu";
        case Mnemonic::kXor: return "xor";
        case Mnemonic::kSrl: return "srl";
        case Mnemonic::kSra: return "sra";
        case Mnemonic::kOr: return "or";
        case Mnemonic::kAnd: return "and";
        case Mnemonic::kFence: return "fence";
        case Mnemonic::kFencei: return "fence.i";
        case Mnemonic::kEcall: return "ecall";
        case Mnemonic::kEbreak: return "ebreak";
        case Mnemonic::kMret: return "mret";
        case Mnemonic::kCsrrw: return "csrrw";
        case Mnemonic::kCsrrs: return "csrrs";
        case Mnemonic::kCsrrc: return "csrrc";
        case Mnemonic::kCsrrwi: return "csrrwi";
        case Mnemonic::kCsrrsi: return "csrrsi";
        case Mnemonic::kCsrrci: return "csrrci";
        case Mnemonic::kMul: return "mul";
        case Mnemonic::kMulh: return "mulh";
        case Mnemonic::kMulhsu: return "mulhsu";
        case Mnemonic::kMulhu: return "mulhu";
        case Mnemonic::kDiv: return "div";
        case Mnemonic::kDivu: return "divu";
        case Mnemonic::kRem: return "rem";
        case Mnemonic::kRemu: return "remu";
        case Mnemonic::kInvalid: return "invalid";
    }
    return "invalid";
}

DecodedInstruction Decoder::Decode(uint32_t word) {
    DecodedInstruction ins;
    ins.raw = word;

    const uint32_t opcode = Bits(word, 6, 0);
    ins.rd = static_cast<uint8_t>(Bits(word, 11, 7));
    ins.funct3 = static_cast<uint8_t>(Bits(word, 14, 12));
    ins.rs1 = static_cast<uint8_t>(Bits(word, 19, 15));
    ins.rs2 = static_cast<uint8_t>(Bits(word, 24, 20));
    ins.funct7 = static_cast<uint8_t>(Bits(word, 31, 25));

    switch (opcode) {
        case kOpLui:
            ins.format = Format::kU;
            ins.mnemonic = Mnemonic::kLui;
            ins.imm = ImmU(word);
            break;

        case kOpAuipc:
            ins.format = Format::kU;
            ins.mnemonic = Mnemonic::kAuipc;
            ins.imm = ImmU(word);
            break;

        case kOpJal:
            ins.format = Format::kJ;
            ins.mnemonic = Mnemonic::kJal;
            ins.imm = ImmJ(word);
            break;

        case kOpJalr:
            ins.format = Format::kI;
            ins.imm = ImmI(word);
            if (ins.funct3 == 0b000) ins.mnemonic = Mnemonic::kJalr;
            break;

        case kOpBranch:
            ins.format = Format::kB;
            ins.imm = ImmB(word);
            switch (ins.funct3) {
                case 0b000: ins.mnemonic = Mnemonic::kBeq; break;
                case 0b001: ins.mnemonic = Mnemonic::kBne; break;
                case 0b100: ins.mnemonic = Mnemonic::kBlt; break;
                case 0b101: ins.mnemonic = Mnemonic::kBge; break;
                case 0b110: ins.mnemonic = Mnemonic::kBltu; break;
                case 0b111: ins.mnemonic = Mnemonic::kBgeu; break;
                default: break;
            }
            break;

        case kOpLoad:
            ins.format = Format::kI;
            ins.imm = ImmI(word);
            switch (ins.funct3) {
                case 0b000:
                    ins.mnemonic = Mnemonic::kLb;
                    ins.mem_size = mem::ElementSize::kByte;
                    ins.sign_extend = true;
                    break;
                case 0b001:
                    ins.mnemonic = Mnemonic::kLh;
                    ins.mem_size = mem::ElementSize::kHalfword;
                    ins.sign_extend = true;
                    break;
                case 0b010:
                    ins.mnemonic = Mnemonic::kLw;
                    ins.mem_size = mem::ElementSize::kWord;
                    break;
                case 0b100:
                    ins.mnemonic = Mnemonic::kLbu;
                    ins.mem_size = mem::ElementSize::kByte;
                    break;
                case 0b101:
                    ins.mnemonic = Mnemonic::kLhu;
                    ins.mem_size = mem::ElementSize::kHalfword;
                    break;
                default: break;
            }
            break;

        case kOpStore:
            ins.format = Format::kS;
            ins.imm = ImmS(word);
            switch (ins.funct3) {
                case 0b000:
                    ins.mnemonic = Mnemonic::kSb;
                    ins.mem_size = mem::ElementSize::kByte;
                    break;
                case 0b001:
                    ins.mnemonic = Mnemonic::kSh;
                    ins.mem_size = mem::ElementSize::kHalfword;
                    break;
                case 0b010:
                    ins.mnemonic = Mnemonic::kSw;
                    ins.mem_size = mem::ElementSize::kWord;
                    break;
                default: break;
            }
            break;

        case kOpImm:
            ins.format = Format::kI;
            ins.imm = ImmI(word);
            switch (ins.funct3) {
                case 0b000: ins.mnemonic = Mnemonic::kAddi; break;
                case 0b010: ins.mnemonic = Mnemonic::kSlti; break;
                case 0b011: ins.mnemonic = Mnemonic::kSltiu; break;
                case 0b100: ins.mnemonic = Mnemonic::kXori; break;
                case 0b110: ins.mnemonic = Mnemonic::kOri; break;
                case 0b111: ins.mnemonic = Mnemonic::kAndi; break;
                case 0b001:
                    if (ins.funct7 == kFunct7Base) ins.mnemonic = Mnemonic::kSlli;
                    break;
                case 0b101:
                    if (ins.funct7 == kFunct7Base) ins.mnemonic = Mnemonic::kSrli;
                    else if (ins.funct7 == kFunct7Alt) ins.mnemonic = Mnemonic::kSrai;
                    break;
                default: break;
            }
            // Shift amount lives in imm[4:0]; SLLI/SRLI/SRAI don't use imm_i's
            // sign extension semantics, but keeping the raw field around too
            // costs nothing and the interpreter can just mask it.
            break;

        case kOpReg:
            ins.format = Format::kR;
            switch (ins.funct7) {
                case kFunct7Base:
                    switch (ins.funct3) {
                        case 0b000: ins.mnemonic = Mnemonic::kAdd; break;
                        case 0b001: ins.mnemonic = Mnemonic::kSll; break;
                        case 0b010: ins.mnemonic = Mnemonic::kSlt; break;
                        case 0b011: ins.mnemonic = Mnemonic::kSltu; break;
                        case 0b100: ins.mnemonic = Mnemonic::kXor; break;
                        case 0b101: ins.mnemonic = Mnemonic::kSrl; break;
                        case 0b110: ins.mnemonic = Mnemonic::kOr; break;
                        case 0b111: ins.mnemonic = Mnemonic::kAnd; break;
                        default: break;
                    }
                    break;
                case kFunct7Alt:
                    switch (ins.funct3) {
                        case 0b000: ins.mnemonic = Mnemonic::kSub; break;
                        case 0b101: ins.mnemonic = Mnemonic::kSra; break;
                        default: break;
                    }
                    break;
                case kFunct7Mul:
                    switch (ins.funct3) {
                        case 0b000: ins.mnemonic = Mnemonic::kMul; break;
                        case 0b001: ins.mnemonic = Mnemonic::kMulh; break;
                        case 0b010: ins.mnemonic = Mnemonic::kMulhsu; break;
                        case 0b011: ins.mnemonic = Mnemonic::kMulhu; break;
                        case 0b100: ins.mnemonic = Mnemonic::kDiv; break;
                        case 0b101: ins.mnemonic = Mnemonic::kDivu; break;
                        case 0b110: ins.mnemonic = Mnemonic::kRem; break;
                        case 0b111: ins.mnemonic = Mnemonic::kRemu; break;
                        default: break;
                    }
                    break;
                default: break;
            }
            break;

        case kOpFence:
            ins.format = Format::kI;
            switch (ins.funct3) {
                case 0b000: ins.mnemonic = Mnemonic::kFence; break;
                case 0b001: ins.mnemonic = Mnemonic::kFencei; break;
                default: break;
            }
        // CSR instructions reuse the I-type field layout (rd, funct3, rs1,
        // imm[11:0]) even though the semantics differ: imm is an unsigned
        // CSR address rather than a sign-extended value, and for the *i
        // variants rs1 holds a 5-bit zero-extended immediate rather than a
        // register number (the interpreter reads it directly off ins.rs1).
        case kOpSystem:
            ins.format = Format::kI;
            switch (ins.funct3) {
                case 0b000: {
                    const uint32_t imm12 = Bits(word, 31, 20);
                    if (imm12 == 0x000) ins.mnemonic = Mnemonic::kEcall;
                    else if (imm12 == 0x001) ins.mnemonic = Mnemonic::kEbreak;
                    else if (imm12 == 0x302) ins.mnemonic = Mnemonic::kMret;
                    break;
                }
                case 0b001: ins.mnemonic = Mnemonic::kCsrrw;  ins.imm = ImmCsr(word); break;
                case 0b010: ins.mnemonic = Mnemonic::kCsrrs;  ins.imm = ImmCsr(word); break;
                case 0b011: ins.mnemonic = Mnemonic::kCsrrc;  ins.imm = ImmCsr(word); break;
                case 0b101: ins.mnemonic = Mnemonic::kCsrrwi; ins.imm = ImmCsr(word); break;
                case 0b110: ins.mnemonic = Mnemonic::kCsrrsi; ins.imm = ImmCsr(word); break;
                case 0b111: ins.mnemonic = Mnemonic::kCsrrci; ins.imm = ImmCsr(word); break;
                default: break;
            }
            break;

        default:
            break;
    }

    return ins;
}

}  // namespace nf::isa
