#ifndef NF_ISA_DECODER_HPP_
#define NF_ISA_DECODER_HPP_

#include <cstdint>
#include <string_view>

#include "nf/mem/memory.hpp"

namespace nf::isa {

// RV32I/M instruction encodings. Order matches the six base formats;
// kInvalid marks a word that didn't decode to a known instruction.
enum class Format : uint8_t { kR, kI, kS, kB, kU, kJ, kInvalid };

enum class Mnemonic : uint8_t {
    // RV32I
    kLui, kAuipc, 
    // Branch and Jump
    kBeq, kBne, kBlt, kBge, kBltu, kBgeu, kJal, kJalr,
    // Load Store
    kLb, kLh, kLw, kLbu, kLhu,
    kSb, kSh, kSw,
    // Arithmetic and logical
    kAddi, kSlti, kSltiu, kXori, kOri, kAndi, kSlli, kSrli, kSrai,
    kAdd, kSub, kSll, kSlt, kSltu, kXor, kSrl, kSra, kOr, kAnd,
    // System
    kFence, kEcall, kEbreak, kMret,
    // Zicsr (minimal subset -- just enough to boot riscv-tests' env/p)
    kCsrrw, kCsrrs, kCsrrc, kCsrrwi, kCsrrsi, kCsrrci,
    // RV32M
    kMul, kMulh, kMulhsu, kMulhu, kDiv, kDivu, kRem, kRemu,

    kInvalid,
};

std::string_view ToString(Mnemonic m);

// Fully identified instruction: which operation, which registers, and the
// sign-extended immediate (format-dependent bit layout already resolved).
// This is the payload the ID pipeline stage will eventually latch into
// the ID/EX register, so decode logic lives here and nowhere else.
struct DecodedInstruction {
    uint32_t raw = 0;
    Format format = Format::kInvalid;
    Mnemonic mnemonic = Mnemonic::kInvalid;

    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t funct3 = 0;
    uint8_t funct7 = 0;
    int32_t imm = 0;

    // Meaningful only for load/store mnemonics; resolved here from funct3
    // so the MEM stage never has to re-interpret raw opcode bits.
    mem::ElementSize mem_size = mem::ElementSize::kWord;

    bool Valid() const { return mnemonic != Mnemonic::kInvalid; }
};

class Decoder {
public:
    static DecodedInstruction Decode(uint32_t word);
};

}  // namespace nf::isa

#endif  // NF_ISA_DECODER_HPP_
