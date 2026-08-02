#include <catch2/catch_test_macros.hpp>

#include "nf/isa/decoder.hpp"

using nf::isa::Decoder;
using nf::isa::Format;
using nf::isa::Mnemonic;

// Encoding helpers mirror the RV32 bit layouts so test cases read like the
// spec tables rather than pre-computed hex.
namespace {

uint32_t encode_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3,
                   uint32_t rd, uint32_t opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) |
           (rd << 7) | opcode;
}

uint32_t encode_i(int32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd,
                   uint32_t opcode) {
    return (static_cast<uint32_t>(imm) << 20) | (rs1 << 15) | (funct3 << 12) |
           (rd << 7) | opcode;
}

uint32_t encode_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3,
                   uint32_t opcode) {
    const uint32_t u = static_cast<uint32_t>(imm);
    return ((u >> 5 & 0x7F) << 25) | (rs2 << 20) | (rs1 << 15) |
           (funct3 << 12) | ((u & 0x1F) << 7) | opcode;
}

uint32_t encode_b(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3,
                   uint32_t opcode) {
    const uint32_t u = static_cast<uint32_t>(imm);
    return (((u >> 12) & 0x1) << 31) | (((u >> 5) & 0x3F) << 25) |
           (rs2 << 20) | (rs1 << 15) | (funct3 << 12) |
           (((u >> 1) & 0xF) << 8) | (((u >> 11) & 0x1) << 7) | opcode;
}

uint32_t encode_u(int32_t imm20, uint32_t rd, uint32_t opcode) {
    return (static_cast<uint32_t>(imm20) & 0xFFFFF000u) | (rd << 7) | opcode;
}

uint32_t encode_j(int32_t imm, uint32_t rd, uint32_t opcode) {
    const uint32_t u = static_cast<uint32_t>(imm);
    return (((u >> 20) & 0x1) << 31) | (((u >> 1) & 0x3FF) << 21) |
           (((u >> 11) & 0x1) << 20) | (((u >> 12) & 0xFF) << 12) |
           (rd << 7) | opcode;
}

}  // namespace

TEST_CASE("Decoder identifies R-type register-register ops", "[decoder]") {
    // add x5, x6, x7
    auto ins = Decoder::Decode(encode_r(0b0000000, 7, 6, 0b000, 5, 0b0110011));
    CHECK(ins.format == Format::kR);
    CHECK(ins.mnemonic == Mnemonic::kAdd);
    CHECK(ins.rd == 5);
    CHECK(ins.rs1 == 6);
    CHECK(ins.rs2 == 7);

    // sub x5, x6, x7 -- same funct3, funct7 selects SUB vs ADD.
    ins = Decoder::Decode(encode_r(0b0100000, 7, 6, 0b000, 5, 0b0110011));
    CHECK(ins.mnemonic == Mnemonic::kSub);

    // mul x5, x6, x7 -- RV32M shares the OP opcode with a distinct funct7.
    ins = Decoder::Decode(encode_r(0b0000001, 7, 6, 0b000, 5, 0b0110011));
    CHECK(ins.mnemonic == Mnemonic::kMul);
}

TEST_CASE("Decoder sign-extends I-type immediates", "[decoder]") {
    // addi x1, x2, -1  (imm = 0xFFF)
    auto ins = Decoder::Decode(encode_i(-1, 2, 0b000, 1, 0b0010011));
    CHECK(ins.format == Format::kI);
    CHECK(ins.mnemonic == Mnemonic::kAddi);
    CHECK(ins.imm == -1);
    CHECK(ins.rs1 == 2);
    CHECK(ins.rd == 1);

    // addi x1, x2, 2047 (max positive 12-bit immediate)
    ins = Decoder::Decode(encode_i(2047, 2, 0b000, 1, 0b0010011));
    CHECK(ins.imm == 2047);
}

TEST_CASE("Decoder distinguishes SRLI/SRAI via funct7 on shift-immediate", "[decoder]") {
    auto srli = Decoder::Decode(encode_i(0, 1, 0b101, 2, 0b0010011));
    CHECK(srli.mnemonic == Mnemonic::kSrli);

    auto srai = Decoder::Decode(encode_i(0b010000000000, 1, 0b101, 2, 0b0010011));
    CHECK(srai.mnemonic == Mnemonic::kSrai);
}

TEST_CASE("Decoder decodes loads and stores with correct immediates", "[decoder]") {
    // lw x3, -4(x2)
    auto lw = Decoder::Decode(encode_i(-4, 2, 0b010, 3, 0b0000011));
    CHECK(lw.format == Format::kI);
    CHECK(lw.mnemonic == Mnemonic::kLw);
    CHECK(lw.imm == -4);

    // sw x3, 8(x2)
    auto sw = Decoder::Decode(encode_s(8, 3, 2, 0b010, 0b0100011));
    CHECK(sw.format == Format::kS);
    CHECK(sw.mnemonic == Mnemonic::kSw);
    CHECK(sw.rs1 == 2);
    CHECK(sw.rs2 == 3);
    CHECK(sw.imm == 8);
}

TEST_CASE("Decoder decodes branches with B-immediate", "[decoder]") {
    // beq x1, x2, -16
    auto ins = Decoder::Decode(encode_b(-16, 2, 1, 0b000, 0b1100011));
    CHECK(ins.format == Format::kB);
    CHECK(ins.mnemonic == Mnemonic::kBeq);
    CHECK(ins.imm == -16);

    // bge x1, x2, 4094 (near-max even B-immediate)
    ins = Decoder::Decode(encode_b(4094, 2, 1, 0b101, 0b1100011));
    CHECK(ins.mnemonic == Mnemonic::kBge);
    CHECK(ins.imm == 4094);
}

TEST_CASE("Decoder decodes U-type LUI/AUIPC", "[decoder]") {
    auto lui = Decoder::Decode(encode_u(static_cast<int32_t>(0xABCDE000), 5, 0b0110111));
    CHECK(lui.format == Format::kU);
    CHECK(lui.mnemonic == Mnemonic::kLui);
    CHECK(lui.imm == static_cast<int32_t>(0xABCDE000));

    auto auipc = Decoder::Decode(encode_u(0x1000, 5, 0b0010111));
    CHECK(auipc.mnemonic == Mnemonic::kAuipc);
}

TEST_CASE("Decoder decodes J-type JAL with correctly-ordered immediate bits", "[decoder]") {
    // jal x1, -1024
    auto ins = Decoder::Decode(encode_j(-1024, 1, 0b1101111));
    CHECK(ins.format == Format::kJ);
    CHECK(ins.mnemonic == Mnemonic::kJal);
    CHECK(ins.imm == -1024);
    CHECK(ins.rd == 1);
}

TEST_CASE("Decoder decodes JALR as I-type", "[decoder]") {
    auto ins = Decoder::Decode(encode_i(4, 1, 0b000, 5, 0b1100111));
    CHECK(ins.format == Format::kI);
    CHECK(ins.mnemonic == Mnemonic::kJalr);
    CHECK(ins.imm == 4);
}

TEST_CASE("Decoder decodes ECALL/EBREAK from the SYSTEM opcode", "[decoder]") {
    auto ecall = Decoder::Decode(encode_i(0, 0, 0b000, 0, 0b1110011));
    CHECK(ecall.mnemonic == Mnemonic::kEcall);

    auto ebreak = Decoder::Decode(encode_i(1, 0, 0b000, 0, 0b1110011));
    CHECK(ebreak.mnemonic == Mnemonic::kEbreak);
}

TEST_CASE("Decoder marks unrecognized encodings invalid", "[decoder]") {
    // opcode 0b1111111 isn't a valid RV32IM major opcode.
    auto ins = Decoder::Decode(0b1111111);
    CHECK_FALSE(ins.Valid());
    CHECK(ins.mnemonic == Mnemonic::kInvalid);
}
