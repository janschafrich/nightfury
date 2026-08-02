#include <catch2/catch_test_macros.hpp>

#include "nf/core/alu.hpp"

using nf::core::AluOp;
using nf::core::Compute;

namespace {
constexpr uint32_t kIntMin = 0x80000000u;   // INT32_MIN bit pattern
constexpr uint32_t kAllOnes = 0xFFFFFFFFu;  // -1 bit pattern
}  // namespace

TEST_CASE("ADD wraps on overflow", "[alu]") {
    CHECK(Compute(AluOp::kAdd, 2, 3) == 5);
    CHECK(Compute(AluOp::kAdd, kAllOnes, 1) == 0);  // -1 + 1 wraps to 0
}

TEST_CASE("SUB wraps on underflow", "[alu]") {
    CHECK(Compute(AluOp::kSub, 5, 3) == 2);
    CHECK(Compute(AluOp::kSub, 0, 1) == kAllOnes);  // 0 - 1 wraps to -1
}

TEST_CASE("Bitwise ops", "[alu]") {
    CHECK(Compute(AluOp::kXor, 0b1100, 0b1010) == 0b0110u);
    CHECK(Compute(AluOp::kOr,  0b1100, 0b1010) == 0b1110u);
    CHECK(Compute(AluOp::kAnd, 0b1100, 0b1010) == 0b1000u);
}

TEST_CASE("Shift amount uses only the low 5 bits of rs2", "[alu]") {
    CHECK(Compute(AluOp::kSll, 1, 4) == 0x10u);
    CHECK(Compute(AluOp::kSll, 1, 32) == 1u);  // shamt = 32 & 0x1F = 0 -> no-op
    CHECK(Compute(AluOp::kSrl, kIntMin, 4) == 0x08000000u);
    CHECK(Compute(AluOp::kSra, kIntMin, 4) == 0xF8000000u);  // sign-extends
    CHECK(Compute(AluOp::kSra, 0x40000000, 4) == 0x04000000u);
}

TEST_CASE("SLT / SLTU disagree on signed vs unsigned comparison", "[alu]") {
    // -1 < 1 is true signed, but 0xFFFFFFFF < 1 is false unsigned
    CHECK(Compute(AluOp::kSlt,  kAllOnes, 1) == 1u);
    CHECK(Compute(AluOp::kSltu, kAllOnes, 1) == 0u);
    CHECK(Compute(AluOp::kSlt,  1, kAllOnes) == 0u);
    CHECK(Compute(AluOp::kSltu, 1, kAllOnes) == 1u);
}

TEST_CASE("BEQ / BNE", "[alu]") {
    CHECK(Compute(AluOp::kBeq, 5, 5) == 1u);
    CHECK(Compute(AluOp::kBeq, 5, 6) == 0u);
    CHECK(Compute(AluOp::kBne, 5, 6) == 1u);
    CHECK(Compute(AluOp::kBne, 5, 5) == 0u);
}

TEST_CASE("BLT / BLTU disagree on signed vs unsigned comparison", "[alu]") {
    // signed: -1 < 0 is true
    CHECK(Compute(AluOp::kBlt, kAllOnes, 0) == 1u);
    // unsigned: 0xFFFFFFFF < 0 is false
    CHECK(Compute(AluOp::kBltu, kAllOnes, 0) == 0u);
}

TEST_CASE("BGE / BGEU disagree on signed vs unsigned comparison", "[alu]") {
    // signed: -1 >= 0 is false
    CHECK(Compute(AluOp::kBge, kAllOnes, 0) == 0u);
    // unsigned: 0xFFFFFFFF >= 0 is true
    CHECK(Compute(AluOp::kBgeu, kAllOnes, 0) == 1u);
}

TEST_CASE("MUL keeps the low 32 bits of the signed product", "[alu]") {
    CHECK(Compute(AluOp::kMul, static_cast<uint32_t>(-3), 4) == static_cast<uint32_t>(-12));
}

TEST_CASE("MULH takes the high 32 bits of a signed x signed product", "[alu]") {
    // (-2^31) * (-2^31) = 2^62; top 32 bits = 0x40000000
    CHECK(Compute(AluOp::kMulh, kIntMin, kIntMin) == 0x40000000u);
}

TEST_CASE("MULHSU takes the high 32 bits of a signed x unsigned product", "[alu]") {
    // rs1 signed (-1) * rs2 unsigned (4294967295) = -4294967295
    // top 32 bits, arithmetic shift => all ones
    CHECK(Compute(AluOp::kMulhsu, kAllOnes, kAllOnes) == kAllOnes);
}

TEST_CASE("MULHU takes the high 32 bits of an unsigned x unsigned product", "[alu]") {
    // 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE00000001
    CHECK(Compute(AluOp::kMulhu, kAllOnes, kAllOnes) == 0xFFFFFFFEu);
}

TEST_CASE("DIV / DIVU basic division", "[alu]") {
    CHECK(Compute(AluOp::kDiv, static_cast<uint32_t>(-12), 4) == static_cast<uint32_t>(-3));
    CHECK(Compute(AluOp::kDivu, 12, 4) == 3u);
}

TEST_CASE("DIV / DIVU by zero return all-ones per the RISC-V M spec", "[alu]") {
    CHECK(Compute(AluOp::kDiv, 12, 0) == kAllOnes);
    CHECK(Compute(AluOp::kDivu, 12, 0) == kAllOnes);
}

TEST_CASE("DIV handles the INT32_MIN / -1 signed overflow case", "[alu]") {
    // Spec-defined: the mathematical result (2^31) doesn't fit in 32 bits,
    // so DIV returns the dividend unchanged instead of trapping/overflowing.
    CHECK(Compute(AluOp::kDiv, kIntMin, kAllOnes) == kIntMin);
}

TEST_CASE("REM / REMU basic remainder", "[alu]") {
    CHECK(Compute(AluOp::kRem, static_cast<uint32_t>(-13), 4) == static_cast<uint32_t>(-1));
    CHECK(Compute(AluOp::kRemu, 13, 4) == 1u);
}

TEST_CASE("REM / REMU by zero return the dividend unchanged per the RISC-V M spec", "[alu]") {
    CHECK(Compute(AluOp::kRem, 13, 0) == 13u);
    CHECK(Compute(AluOp::kRemu, 13, 0) == 13u);
}

TEST_CASE("REM handles the INT32_MIN / -1 signed overflow case", "[alu]") {
    CHECK(Compute(AluOp::kRem, kIntMin, kAllOnes) == 0u);
}
