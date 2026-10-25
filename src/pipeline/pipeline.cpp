#include "nf/pipeline/pipeline.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/stages.hpp"

namespace nf::pipeline {


// using nf::pipeline::PipelineLatch;
// using nf::pipeline::FetchStage;
// using nf::pipeline::FetchPacket;

// owns the 
// - stages
// - latches 
// - control, two phase tick

Pipeline::Pipeline( mem::MemoryInterface &memory, 
                    core::ArchState &arch, 
                    uint32_t start_pc, 
                    uint32_t tohost_addr
) :
    memory_(memory),
    regfile_(arch.regs),
    csrs_(arch.csrs),
    tohost_addr_(tohost_addr)
{
    pc_.Set(start_pc);
    pc_.Advance();
}

void Pipeline::Tick() {
    // Evaluates all pipeline stages, then advances to the next cycle
    // 1. Control pass WB -> IF, any stalls in the pipeline?
    // fetch is always ready
    const bool de_ready     = de_.Ready();
    // const bool ex_ready     = ex_.Ready();
    // const bool mem_ready    = mem_.Ready();
    // const bool wb_ready     = wb_.Ready();

    // 2. Data pass - compute the next output
    fe_.Evaluate(pc_, if_id_, memory_, de_ready);

    // 3. Advance - register the new values for the next cycle
    // pc_.Advance();
    // if_id_.Advance();
    // id_ex_.Advance();
    // ex_mem_.Advance();
    // mem_wb_.Advance();
}





}  // namespace nf::pipeline
