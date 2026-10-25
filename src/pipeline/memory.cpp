#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"


namespace nf::pipeline {
    // load store
    // wb mux
    // halt detect
Memory::Evaluate(
    const PipelineLatch<ExecutePacket>  &ex_mem,
    PipelineLatch<MemoryPacket>         &mem_wb,
    mem::MemoryInterface                &memory,
    const bool                          csm_ready
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

    if (ex_mem.is_load) {
        pkt.pc      = ex_mem.pc;
        pkt.wb_data = memory.Load(ex_mem.mem_size, ex_mem.alu_result);
        pkt.rd      = ex_mem.rd;
        pkt.csr     = ex_mem.csr;
    }
    else if (ex_mem.is_store) {
        memory.Store(ex_mem.mem_size, ex_mem.alu_result, ex_mem.store_val);

        pkt.pc      = ex_mem.pc;
        pkt.wb_data = 0;
        pkt.rd      = 0;
        pkt.csr     = ex_mem.csr;
    }
    else {
        pkt.pc      = ex_mem.pc;
        pkt.wb_data = ex_mem.alu_result;
        pkt.rd      = ex_mem.rd;
        pkt.csr     = ex_mem.csr;
    }

    mem_wb.SetNext(pkt);
    
    return;
}

} // namespace
