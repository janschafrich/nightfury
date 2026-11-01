#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"


namespace nf::pipeline {
    // load store
    // wb mux
    // halt detect
void MemoryStage::Evaluate(
    PipelineLatch<ExecutePacket>  &ex_mem,
    PipelineLatch<MemoryPacket>   &mem_wb,
    mem::MemoryInterface          &memory,
    const bool                    csm_ready
){
    if (!csm_ready) {
        ex_mem.Stall();
        return;
    }

    if (!ex_mem.Valid()) {
        mem_wb.SetNop();
        return;
    }
    
    MemoryPacket pkt;

    if (ex_mem.Q().is_load) {
        pkt.pc      = ex_mem.Q().pc;
        pkt.wb_data = memory.Load(ex_mem.Q().mem_size, ex_mem.Q().alu_result);
        pkt.rd      = ex_mem.Q().rd;
        pkt.csr     = ex_mem.Q().csr;
    }
    else if (ex_mem.Q().is_store) {
        memory.Store(ex_mem.Q().mem_size, ex_mem.Q().alu_result, ex_mem.Q().store_val);

        pkt.pc      = ex_mem.Q().pc;
        pkt.wb_data = 0;
        pkt.rd      = 0;
        pkt.csr     = ex_mem.Q().csr;
    }
    else {
        pkt.pc      = ex_mem.Q().pc;
        pkt.wb_data = ex_mem.Q().alu_result;
        pkt.rd      = ex_mem.Q().rd;
        pkt.csr     = ex_mem.Q().csr;
    }

    mem_wb.Set(pkt);
    
    return;
}

} // namespace
