#include <cassert>

#include "nf/core/interpreter.hpp"
#include "nf/core/alu.hpp"

namespace nf::core {

using nf::isa::Mnemonic;
using nf::core::AluOp;
using nf::mem::ElementSize;

Interpreter::Interpreter(mem::MemoryInterface &memory, uint32_t reset_pc)
     : memory_(memory), pc_(reset_pc) {}

StepResult Interpreter::ProcessInstruction() {
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

        case Mnemonic::kCsrrw: case Mnemonic::kCsrrs: case Mnemonic::kCsrrc:
        case Mnemonic::kCsrrwi: case Mnemonic::kCsrrsi: case Mnemonic::kCsrrci: {
            const uint16_t csr_addr = static_cast<uint16_t>(inst_.imm);
            const uint32_t old = csrs_.Read(csr_addr);

            // *i variants take a 5-bit zero-extended immediate straight off
            // the rs1 field -- it's not a register number for these (see
            // decoder.cpp); register variants use rs1_val like any other op.
            const bool is_immediate_form =
                inst_.mnemonic == Mnemonic::kCsrrwi ||
                inst_.mnemonic == Mnemonic::kCsrrsi ||
                inst_.mnemonic == Mnemonic::kCsrrci;
            const uint32_t operand = is_immediate_form ? inst_.rs1 : rs1_val;

            uint32_t new_val = old;
            switch (inst_.mnemonic) {
                case Mnemonic::kCsrrw: case Mnemonic::kCsrrwi: new_val = operand; break;
                case Mnemonic::kCsrrs: case Mnemonic::kCsrrsi: new_val = old | operand; break;
                case Mnemonic::kCsrrc: case Mnemonic::kCsrrci: new_val = old & ~operand; break;
                default: break;
            }
            // Spec technically skips the write for csrrs/csrrc(i) when the
            // operand is zero (e.g. `csrr t5, mcause` expands to
            // `csrrs t5, mcause, x0`). Skipped here: old | 0 == old and
            // old & ~0 == old, so writing unconditionally is observationally
            // identical for any CSR without read side effects -- and
            // CsrFile has none (see csr_file.hpp).
            csrs_.Write(csr_addr, new_val);
            regfile_.Write(inst_.rd, old);
            break;
        }

        case Mnemonic::kEcall:
            // riscv-tests' env/p never leaves M-mode (RVTEST_RV32U/RV32UM's
            // `init` macro is empty), so this interpreter -- modeling a
            // single always-M-mode hart -- always traps with the M-mode
            // ecall cause. trap_vector (env/p/riscv_test.h) dispatches on
            // mcause to reach write_tohost.
            csrs_.set_mepc(pc_);
            csrs_.set_mcause(kCauseMachineEcall);
            pc_next = csrs_.mtvec();
            break;

        case Mnemonic::kMret:
            pc_next = csrs_.mepc();
            break;

        case Mnemonic::kFence: case Mnemonic::kEbreak:
            break;  // no architectural effect modeled yet

        case Mnemonic::kInvalid:
            assert(false && "Decode produced kInvalid on a fetched word");
            break;
    }

    // TODO: detect the write_tohost store (env/p/riscv_test.h) and return
    // StepResult::kHalted -- needs the interpreter to know the tohost
    // address, which the loader doesn't surface yet (it only reads program
    // headers, not the symbol table). Design this once LoadElf grows symbol
    // lookup.
    pc_ = pc_next;

    return StepResult::kOk;
}

}  // namespace nf::core
