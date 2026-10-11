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

Pipeline::Pipeline(uint32_t start_pc) {
    pc_.SetNext(start_pc);
    pc_.Commit();
}

void Pipeline::Tick() {
    // 1. Control pass WB -> IF
    // const bool wb_ready     = wb_.Ready();
    // const bool mem_ready    = mem_.Ready();
    // const bool ex_ready     = ex_.Ready();
    // const bool de_ready     = de_.Ready();
    // fetch is always ready

    // 2. Data pass
    // fe_.Evaluate(pc, if_id_, de_ready, mem_)

    // 3. Commit 
    // pc_.Commit();
    // if_id_.Commit();
    // id_ex_.Commit();
    // ex_mem_.Commit();
    // mem_wb_.Commit();
}





}  // namespace nf::pipeline
