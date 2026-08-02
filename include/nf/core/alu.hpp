#ifndef NF_CORE_ALU_HPP_
#define NF_CORE_ALU_HPP_

#include <cstdint>

namespace nf::core {

enum class AluOp : uint8_t {
    kAdd, kSub, kSll, kSlt, kSltu, kSrl, kSra, kXor, kOr, kAnd,
    kBeq, kBne, kBlt, kBge, kBltu, kBgeu,
    kMul, kMulh, kMulhsu, kMulhu, kDiv, kDivu, kRem, kRemu
};

// Avoid any state, implement as function instead of class
uint32_t Compute(AluOp op, uint32_t a, uint32_t b);

}  // namespace nf::core

#endif  // NF_CORE_ALU_HPP_