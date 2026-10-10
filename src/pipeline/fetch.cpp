#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/pipeline/mem/memory.hpp"

// fetch PC
// inject bubbles on stall

bool is_control(uint32_t instr) {

}

namespace nf::pipeline {

using nf::pipeline::PipelineLatch;
using nf::mem::ElementSize;

FetchStage::Evaluate(
    PipelineLatch<uint32_t>     &pc, 
    PipelineLatch<FetchPacket>  &out,
    bool                        stall,  // set by simulator to pause, decoder for back pressure?
    mem::MemoryInterface        &mem, 
) {

    if (!stall) {
        uint32_t pc_next = pc;  // keep a copy of the pc in case we stall
    }

    uint32_t raw_instr = Memory.Load(ElementSize::kWord, pc);


    // stall for two cycles if control
    stall = stall | is_control(instr);


    // construct fetch packet
    FetchPacket fp = {
        .pc = pc,
        .raw_word = Memory.Load(ElementSize::kWord, pc),
        .valid = !stall     // stall if control instruction
    };

    out.SetNext(FetchPacket);
}


}
