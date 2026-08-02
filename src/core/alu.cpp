#include <cassert>

#include "nf/core/alu.hpp"
#include "nf/isa/decoder.hpp"

using nf::isa::Mnemonic;

namespace nf::core {

constexpr uint32_t kOverflow = 0xFFFFFFFF;
constexpr int32_t kInt32Min = 0x80000000;   // -2^31

AluOp ToAluOp(Mnemonic m) {
    switch (m) {
        case Mnemonic::kAdd:  case Mnemonic::kAddi:  return AluOp::kAdd;
        case Mnemonic::kSlt:  case Mnemonic::kSlti:  return AluOp::kSlt;
        case Mnemonic::kSltu: case Mnemonic::kSltiu: return AluOp::kSltu;
        case Mnemonic::kXor:  case Mnemonic::kXori:  return AluOp::kXor;
        case Mnemonic::kOr:   case Mnemonic::kOri:   return AluOp::kOr;
        case Mnemonic::kAnd:  case Mnemonic::kAndi:  return AluOp::kAnd;
        case Mnemonic::kSll:  case Mnemonic::kSlli:  return AluOp::kSll;
        case Mnemonic::kSrl:  case Mnemonic::kSrli:  return AluOp::kSrl;
        case Mnemonic::kSra:  case Mnemonic::kSrai:  return AluOp::kSra;
        case Mnemonic::kSub:  return AluOp::kSub;      // no SUBI: negate the immediate instead
        // Branch
        case Mnemonic::kBeq:     return AluOp::kBeq;
        case Mnemonic::kBne:     return AluOp::kBne;
        case Mnemonic::kBlt:     return AluOp::kBlt;
        case Mnemonic::kBge:     return AluOp::kBge;
        case Mnemonic::kBltu:    return AluOp::kBltu;
        case Mnemonic::kBgeu:    return AluOp::kBgeu;
        // MUL/DIV family map 1:1, no I-type counterpart exists
        case Mnemonic::kMul:     return AluOp::kMul;
        case Mnemonic::kMulh:    return AluOp::kMulh;
        case Mnemonic::kMulhsu:  return AluOp::kMulhsu;
        case Mnemonic::kMulhu:   return AluOp::kMulhu;
        case Mnemonic::kDiv:     return AluOp::kDiv;
        case Mnemonic::kDivu:    return AluOp::kDivu;
        case Mnemonic::kRem:     return AluOp::kRem;
        case Mnemonic::kRemu:    return AluOp::kRemu;
    }
    assert(false && "unhandled Mnemonic in ToAluOp");
}

uint32_t Compute(AluOp op, uint32_t a, uint32_t b) {
    switch (op) {
        // Arithmetic
        case AluOp::kAdd: return a + b;
        case AluOp::kSub: return a - b;
        // Branch
        case AluOp::kBeq:  return a == b;
        case AluOp::kBne:  return a != b;
        case AluOp::kBlt:  return static_cast<int32_t>(a) < static_cast<int32_t>(b);
        case AluOp::kBge:  return static_cast<int32_t>(a) >= static_cast<int32_t>(b);
        case AluOp::kBltu: return a < b;
        case AluOp::kBgeu: return a >= b;
        // Logical
        case AluOp::kSlt:  return static_cast<int32_t>(a) < static_cast<int32_t>(b);
        case AluOp::kSltu: return a < b;
        case AluOp::kXor: return a ^ b;
        case AluOp::kOr: return a | b;
        case AluOp::kAnd: return a & b;
        case AluOp::kSll: return a << (b & 0x1F);  // shift amount is 5 bit field
        case AluOp::kSrl: return a >> (b & 0x1F);  // shift amount is 5 bit field
        case AluOp::kSra: return static_cast<int32_t>(a) >> (b & 0x1F);  // arithmetic shift requires signed numbers
        
        // Mul lower half
        case AluOp::kMul:  return static_cast<int32_t>(a) * static_cast<int32_t>(b);
        // Mul higher half
        case AluOp::kMulh: // signed x signed = signed
            return static_cast<int64_t>(static_cast<int32_t>(a)) 
                 * static_cast<int64_t>(static_cast<int32_t>(b)) >> 32;
        case AluOp::kMulhsu: // signed x unsigned = signed
            return static_cast<int64_t>(static_cast<int32_t>(a)) 
                 * static_cast<int64_t>(b) >> 32; 
        case AluOp::kMulhu: // unsigned x unsigned = unsigned
            return static_cast<uint32_t>((static_cast<uint64_t>(a) 
                 * static_cast<uint64_t>(b)) >> 32);
        
        case AluOp::kDiv:
            if (b == 0) return 0xFFFFFFFF;
            if (a == kInt32Min && b == kOverflow) return kInt32Min;
            return static_cast<uint32_t>(static_cast<int32_t>(a) / static_cast<int32_t>(b));
        case AluOp::kDivu: 
            if (b == 0) return 0xFFFFFFFF; 
            // overflow case not relevant  see RISCV M spec
            return a / b;
        case AluOp::kRem:
            if (b == 0) return a;   
            if (a == kInt32Min && b == kOverflow) return 0;
            return static_cast<uint32_t>(static_cast<int32_t>(a) % static_cast<int32_t>(b));
        case AluOp::kRemu:
            if (b == 0) return a;
            // overflow case not relevant, see RISCV M spec
            return a % b;
    }
    assert(false && "unhandled AluOp in Compute");  // check compiled into binary (for debug builds)
}

}  // namespace nf::core