#ifndef NF_PIPELINE_PIPELINE_HPP_
#define NF_PIPELINE_PIPELINE_HPP_

#include "nf/pipeline/stages.hpp"
#include "nf/core/arch_state.hpp"
#include "nf/core/step_result.hpp"
#include "nf/pipeline/payloads.hpp"


namespace nf::pipeline {

using nf::pipeline::PipelineLatch;
// using nf::pipline::FetchStage;
// using nf::pipline::DecodeStage;
// using nf::pipline::ExecuteStage;
// using nf::pipline::MemoryStage;
// using nf::pipline::ExecuteStage;

WbAction ToWbAction (nf::isa::Mnemonic m);

class Pipeline {
    public:
    Pipeline(
        nf::mem::MemoryInterface &memory,
        nf::core::ArchState &arch,
        uint32_t start_pc, 
        uint32_t tohost_addr);
        nf::core::StepResult Tick();            // Evaluate IF..WB, then commit latches
        bool Halted() const;
    
    private:
        PipelineLatch<uint32_t>                   pc_;
        PipelineLatch<pipeline::FetchPacket>      if_id_;
        PipelineLatch<pipeline::DecodePacket>     id_ex_;
        PipelineLatch<pipeline::ExecutePacket>    ex_mem_;
        PipelineLatch<pipeline::MemoryPacket>     mem_wb_;

        nf::pipeline::FetchStage                  fe_;
        nf::pipeline::DecodeStage                 de_;
        nf::pipeline::ExecuteStage                ex_;
        nf::pipeline::MemoryStage                 mem_;
        nf::pipeline::WritebackStage              wb_;
        nf::mem::MemoryInterface                  &memory_;       // shared with interpreter
        nf::core::RegisterFile                    &regfile_;      // shared with interpreter
        nf::core::CsrFile                         &csrs_;         // shared with interpreter
        uint32_t                                  tohost_addr_;
    };

// evaluate each pipeline stage
// commit the result for each pipeline stage


}  // namespace nf::pipeline

#endif  // NF_PIPELINE_PIPELINE_HPP_
