#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/mem/memory.hpp"

#include <cassert>

// fetch PC
// inject bubbles on stall

// bool is_control(uint32_t instr) {

// }

namespace nf::pipeline {

using nf::pipeline::PipelineLatch;
using nf::mem::ElementSize;

void FetchStage::Evaluate(
    PipelineLatch<uint32_t>     &pc, 
    PipelineLatch<FetchPacket>  &if_id,
    const mem::MemoryInterface  &mem, 
    const bool                  csm_ready  // back pressure
) {
    assert(pc.Valid());

    if (!csm_ready) {
        pc.Stall();
        // if_id.Stall();          // why would we stall the consumer?
        return;
    }
    
    // construct fetch packet
    FetchPacket pkt;
    pkt.pc          = pc.Q();
    pkt.raw_word    = mem.Load(ElementSize::kWord, pc.Q());

    
    if (redirect_.valid) {
        pc.Set(redirect_.pc);
    } else {
        pc.Set(pc.Q() + 4);      
    }    
    
    if_id.Set(pkt);
}

}
