#ifndef NF_PIPELINE_STAGES_HPP_
#define NF_PIPELINE_STAGES_HPP_

#include "nf/mem/memory.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"

namespace nf::pipeline {

using nf::pipeline::PipelineLatch;
using nf::pipeline::FetchPacket;

class FetchStage {
    public:
        void Evaluate (
            PipelineLatch<uint32_t>     &pc, 
            PipelineLatch<FetchPacket>  &if_id,
            bool                        stall,
            const mem::MemoryInterface  &Memory,
        );
};





} // namespace


#endif // NF_PIPELINE_STAGES_HPP_