#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/core/alu.hpp"
#include "nf/core/csr_file.hpp"
#include "nf/core/csr_update.hpp"
#include "nf/isa/decoder.hpp"

namespace nf::pipeline {

    // ALU
    // branch resolution
    // CSR old value?
Redirect ExecuteStage::Evaluate(
    PipelineLatch<DecodePacket>   &id_ex,
    PipelineLatch<ExecutePacket>  &ex_mem,
    const nf::core::CsrFile       &csrs,
    const bool                    csm_ready
) {
    Redirect redirect{};
    DecodedInstruction inst = id_ex.Q().decoded_inst;
    uint32_t alu_result;
    uint32_t pc_next;

    if (!csm_ready) {
        id_ex.Stall();
        return {redirect.valid  = false};
    }

    if (!id_ex.Valid()){
        ex_mem.SetNop();
        return {redirect.valid  = false};
    }


    if (inst.is_load || inst.is_store) {
        alu_result = id_ex.Q().rs1_val + inst.imm;        // RISCV spec section 2.6
    } 
    else if (inst.is_control) {
        // JAL // RISCV spec section 2.5.1
        if (inst.mnemonic == isa::Mnemonic::kJal) {
            redirect.valid  = true;
            redirect.pc     = id_ex.Q().pc + inst.imm;
            alu_result      = id_ex.Q().pc + 4;
        } 
        else if (inst.mnemonic == isa::Mnemonic::kJalr) {
            redirect.valid  = true;
            redirect.pc     = id_ex.Q().rs1_val + inst.imm;              
            alu_result      = id_ex.Q().pc + 4;
                        
        } 
        else if (inst.format == isa::Format::kB) {
            nf::core::AluOp alu_op = nf::core::ToAluOp(inst.mnemonic);
            redirect.valid         = nf::core::Compute(alu_op, id_ex.Q().rs1_val, id_ex.Q().rs2_val);
            redirect.pc            = id_ex.Q().pc + inst.imm;
        }
    }
    else {
        nf::core::AluOp alu_op = nf::core::ToAluOp(inst.mnemonic);
        alu_result             = nf::core::Compute(alu_op, id_ex.Q().rs1_val, id_ex.Q().rs2_val);
    }

    uint16_t csr_addr = id_ex.Q().csr.addr;
    uint32_t csr_old_val = csrs.Read(csr_addr);
    uint32_t csr_operand = id_ex.Q().csr.val;
    
    ExecutePacket pkt;
    CsrPayload    csr;
    pkt.pc          = id_ex.Q().pc;
    pkt.alu_result  = alu_result;
    pkt.store_val   = id_ex.Q().rs2_val;    // RISCV spec 2.6
    pkt.rd          = inst.rd;
    pkt.mem_size    = inst.mem_size;
    pkt.is_control  = inst.is_control;     
    pkt.is_load     = inst.is_load;
    pkt.is_store    = inst.is_store;
    pkt.sign_extend = inst.sign_extend;
    pkt.csr.action  = id_ex.Q().csr.action;
    pkt.csr.addr    = id_ex.Q().csr.addr;
    pkt.csr.val     = nf::core::CsrUpdate(inst.mnemonic, csr_old_val, csr_operand);
    ex_mem.Set(pkt);

    return redirect;
}


}   // namespace
