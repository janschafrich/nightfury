#include <cassert>

#include "nf/core/interpreter.hpp"
#include "nf/core/alu.hpp"

namespace nf::core {

using nf::isa::Mnemonic;
using nf::core::AluOp;
using nf::mem::ElementSize;

Interpreter::Interpreter(mem::MemoryInterface &memory, uint32_t reset_pc)
     : memory_(memory), pc_(reset_pc) {}

void Interpreter::ProcessInstruction() {
    // Inputs: instruction, register file and a memory
    // Output: architectural effect
    // No timing! (single stage / combinatorial)
    
    // Fetch
    const uint32_t word = memory_.Load(ElementSize::kWord, pc_);
    
    // Decode
    inst_ = isa::Decoder::Decode(word);

    const uint32_t rs1_val = regfile_.Read(inst_.rs1);
    const uint32_t rs2_val = regfile_.Read(inst_.rs2);
    uint32_t pc_next = pc_ + 4;    // default, override on branch

    switch (inst_.mnemonic) {
        // R-type and I-type ALU ops share one ToAluOp table; only the
        // second operand differs (rs2 vs. sign-extended immediate).
        case Mnemonic::kAdd:  case Mnemonic::kSub:  case Mnemonic::kSll:
        case Mnemonic::kSlt:  case Mnemonic::kSltu: case Mnemonic::kXor:
        case Mnemonic::kSrl:  case Mnemonic::kSra:  case Mnemonic::kOr:
        case Mnemonic::kAnd:
        case Mnemonic::kAddi: case Mnemonic::kSlti: case Mnemonic::kSltiu:
        case Mnemonic::kXori: case Mnemonic::kOri:  case Mnemonic::kAndi:
        case Mnemonic::kSlli: case Mnemonic::kSrli: case Mnemonic::kSrai:
        
        case Mnemonic::kMul:  case Mnemonic::kMulh: case Mnemonic::kMulhsu:
        case Mnemonic::kMulhu: case Mnemonic::kDiv:  case Mnemonic::kDivu:
        case Mnemonic::kRem:  case Mnemonic::kRemu: {
            const uint32_t b = (inst_.format == isa::Format::kR)
                                    ? rs2_val
                                    : static_cast<uint32_t>(inst_.imm);
            regfile_.Write(inst_.rd, Compute(ToAluOp(inst_.mnemonic), rs1_val, b));
            break;
        }
        case Mnemonic::kLui:
            regfile_.Write(inst_.rd, static_cast<uint32_t>(inst_.imm));
            break;

        case Mnemonic::kAuipc:
            regfile_.Write(inst_.rd, pc_ + static_cast<uint32_t>(inst_.imm));
            break;

        case Mnemonic::kBeq: case Mnemonic::kBne: case Mnemonic::kBlt:
        case Mnemonic::kBge: case Mnemonic::kBltu: case Mnemonic::kBgeu:
            if (Compute(ToAluOp(inst_.mnemonic), rs1_val, rs2_val)) {
                pc_next = pc_ + static_cast<uint32_t>(inst_.imm);
            }
            break;

        case Mnemonic::kJal:
            regfile_.Write(inst_.rd, pc_ + 4);
            pc_next = pc_ + static_cast<uint32_t>(inst_.imm);
            break;

        case Mnemonic::kJalr:
            regfile_.Write(inst_.rd, pc_ + 4);
            pc_next = (rs1_val + static_cast<uint32_t>(inst_.imm)) & ~1u;
            break;

        case Mnemonic::kLb: case Mnemonic::kLh: case Mnemonic::kLw:
        case Mnemonic::kLbu: case Mnemonic::kLhu: {
            const uint32_t addr = rs1_val + static_cast<uint32_t>(inst_.imm);
            const uint32_t loaded = memory_.Load(inst_.mem_size, addr);

            // Memory::Load always zero-extends the bytes it reads; LB/LH
            // additionally sign-extend per the RV32I semantics, LBU/LHU/LW
            // use the zero-extended value as-is.
            uint32_t result = loaded;
            if (inst_.mnemonic == Mnemonic::kLb) {
                result = static_cast<uint32_t>(static_cast<int32_t>(loaded << 24) >> 24);
            } else if (inst_.mnemonic == Mnemonic::kLh) {
                result = static_cast<uint32_t>(static_cast<int32_t>(loaded << 16) >> 16);
            }
            regfile_.Write(inst_.rd, result);
            break;
        }

        case Mnemonic::kSb: case Mnemonic::kSh: case Mnemonic::kSw: {
            const uint32_t addr = rs1_val + static_cast<uint32_t>(inst_.imm);
            memory_.Store(inst_.mem_size, addr, rs2_val);
            break;
        }

        case Mnemonic::kFence: case Mnemonic::kEcall: case Mnemonic::kEbreak:
            break;  // no architectural effect modeled yet

        case Mnemonic::kInvalid:
            assert(false && "Decode produced kInvalid on a fetched word");
            break;
    }

    pc_ = pc_next;

}

}  // namespace nf::core
