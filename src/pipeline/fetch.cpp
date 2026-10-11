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
    bool                        csm_ready,  // back pressure
    const Redirect              &redirect, 
    mem::MemoryInterface        &mem 
) {
    assert(pc.IsValid());

    if (!csm_ready) {
        pc.SetHold();
        if_id.SetHold();
        return;
    }
    
    // construct fetch packet
    FetchPacket pkt;
    pkt.pc          = pc.Output();
    pkt.raw_word    = mem.Load(ElementSize::kWord, pc.Output());

    
    if (redirect.valid) {
        pc.SetNext(redirect.pc);
    } else {
        pc.SetNext(pc.Output() + 4);      
    }    
    
    if_id.SetNext(pkt);
}

}
