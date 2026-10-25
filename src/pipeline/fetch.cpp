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
    const PipelineLatch<uint32_t>   &pc, 
    PipelineLatch<FetchPacket>      &if_id,
    const mem::MemoryInterface      &mem, 
    const Redirect                  &redirect,
    const bool                      csm_ready  // back pressure
) {
    assert(pc.Valid());

    if (!csm_ready) {
        pc.Stall();
        return;
    }

    const uint32_t fetch_pc = redirect.valid ? redirect.pc : pc.Q();
    
    // construct fetch packet
    FetchPacket pkt;
    pkt.pc          = fetch_pc;
    pkt.raw_word    = mem.Load(ElementSize::kWord, fetch_pc);

    pc.Set(fetch_pc + 4);         
    if_id.Set(pkt);

    return;
}

} // namespace
