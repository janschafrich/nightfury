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
    const bool                    csm_ready,
    const uint32_t                tohost_addr   // keep stage stateless: pass explicitly instead of relying on member variable, unit tests can call Evaluate, without having to instantiate a pipeline first
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

    uint32_t addr = ex_mem.Q().alu_result;

    if (ex_mem.Q().is_load) {
        pkt.pc      = ex_mem.Q().pc;
        pkt.wb_data = memory.Load(ex_mem.Q().mem_size, addr);
        pkt.rd      = ex_mem.Q().rd;
        pkt.csr     = ex_mem.Q().csr;
    }
    else if (ex_mem.Q().is_store) {
        memory.Store(ex_mem.Q().mem_size, addr, ex_mem.Q().store_val);

        pkt.pc      = ex_mem.Q().pc;
        pkt.wb_data = 0;
        pkt.rd      = 0;
        pkt.csr     = ex_mem.Q().csr;

        // End simulation on store to specific address
        if (addr == tohost_addr) {
            pkt.halt = true;
        }
    }
    else {
        pkt.pc      = ex_mem.Q().pc;
        pkt.wb_data = ex_mem.Q().alu_result;
        pkt.rd      = ex_mem.Q().rd;
        pkt.csr     = ex_mem.Q().csr;
        pkt.halt    = false;
    }

    mem_wb.Set(pkt);
    
    return;
}

} // namespace
