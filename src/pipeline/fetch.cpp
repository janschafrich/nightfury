#include "nf/pipeline/fetch.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/pipeline/mem/memory.hpp"

// fetch PC
// inject bubbles on stall

namespace nf::pipeline {

using nf::pipeline::PipelineLatch;
using nf::mem::ElementSize;

FetchStage::Evaluate(
    mem::MemoryInterface        &Memory, 
    PipelineLatch<uint32_t>     pc, 
    PipelineLatch<FetchPacket>  &out
) {

    // construct fetch packet
    FetchPacket fp = {
        .pc = pc,
        .raw_word = Memory.Load(ElementSize::kWord, pc),
        .valid = true
    };

    out.SetNext(FetchPacket);
}


}
