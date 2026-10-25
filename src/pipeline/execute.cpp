#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/core/alu.hpp"
#include "nf/isa/decoder.hpp"

namespace nf::pipeline {

    // ALU
    // branch resolution
    // CSR old value?
ExecuteStage::Evaluate(
    const PipelineLatch<DecodePacket>   &id_ex,
    PipelineLatch<ExecutePacket>        &ex_mem,
    const bool                          csm_ready
) {
    if (!csm_ready) {
        id_ex.Stall();
        return;
    }

    if (!id_ex.Valid()){
        ex_mem.SetNop();
        return;
    }

    DecodedInstruction inst = id_ex.decoded_inst;
    Redirect redirect{};
    uint32_t alu_result;
    uint32_t pc_next;

    if (inst.is_load || inst.is_store) {
        alu_result = rs1_val + inst.imm;        // RISCV spec section 2.6
    } 
    else if (inst.is_control) {
        // JAL // RISCV spec section 2.5.1
        if (inst.mnemonic == isa::Mnemonic::kJal) {
            redirect.valid  = true;
            redirect.pc     = id_ex.pc + inst.imm;
            alu_result      = id_ex.pc + 4;
        } 
        else if (inst.mnemonic == isa::Mnemonic::kJalr) {
            redirect.valid  = true;
            redirect.pc     = id_ex.rs1_val + inst.imm;              
            alu_result      = id_ex.pc + 4;
                        
        } 
        else if (inst.format == isa::Format::kB) {
            AluOp alu_op    = ToAluOp(inst.mnemonic);
            redirect.valid  = Compute(alu_op, id_ex.rs1_val, id_ex.rs2_val);
            redirect.pc     = id_ex.pc + inst.imm;
        }
    }
    else {
        AluOp alu_op = ToAluOp(inst.mnemonic);
        alu_result = Compute(alu_op, id_ex.rs1_val, id_ex.rs2_val);
    }
    
    ExecutePacket pkt;
    pkt.pc          = id_ex.pc;
    pkt.alu_result  = alu_result;
    pkt.store_val   = id_ex.rs2_val;    // RISCV spec 2.6
    // pkt.pc_next     = pc_next;
    pkt.rd          = inst.rd;
    pkt.mem_size    = inst.mem_size;
    pkt.is_control  = inst.is_control;     
    pkt.is_load     = inst.is_load;
    pkt.is_store    = inst.is_store;
    pkt.sign_extend = inst.sign_extend;
    pkt.csr         = 0;    // TODO 

    ex_mem.SetNext(pkt);

    return redirect;
}


}   // namespace
