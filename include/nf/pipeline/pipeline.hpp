#ifndef NF_PIPELINE_PIPELINE_HPP_
#define NF_PIPELINE_PIPELINE_HPP_

#include "nf/pipeline/stages.hpp"

namespace nf::pipeline {

using nf::pipeline::PipelineLatch;
// using nf::pipline::FetchStage;
// using nf::pipline::DecodeStage;
// using nf::pipline::ExecuteStage;
// using nf::pipline::MemoryStage;
// using nf::pipline::ExecuteStage;

class Pipeline {
    public:
        Pipeline(
            mem::MemoryInterface &memory,
            core::ArchState &arch,
            uint32_t start_pc, 
            uint32_t tohost_addr);
        void Tick();            // Evaluate IF..WB, then commit latches
        bool Halted() const;
    
    private:
        PipelineLatch<uint32_t>         pc_;
        PipelineLatch<pipeline::FetchPacket>      if_id_;
        PipelineLatch<pipeline::DecodePacket>     id_ex_;
        PipelineLatch<pipeline::ExecutePacket>    ex_mem_;
        PipelineLatch<pipeline::MemoryPacket>     mem_wb_;

        pipeline::FetchStage                      fe_;
        pipeline::DecodeStage                     de_;
        pipeline::ExecuteStage                    ex_;
        pipeline::MemoryStage                     mem_;
        pipeline::WritebackStage                  wb_;
        mem::MemoryInterface                      &memory_;       // shared with interpreter
        core::RegisterFile                        &regfile_;      // shared with interpreter
        core::CsrFile                             &csrs_;         // shared with interpreter
        uint32_t                                  tohost_addr_;
    };

// evaluate each pipeline stage
// commit the result for each pipeline stage


}  // namespace nf::pipeline

#endif  // NF_PIPELINE_PIPELINE_HPP_
