#ifndef NF_PIPELINE_STAGES_HPP_
#define NF_PIPELINE_STAGES_HPP_

#include "nf/mem/memory.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/core/register_file.hpp"
#include "nf/core/arch_state.hpp"

namespace nf::pipeline {

using nf::pipeline::PipelineLatch;
using nf::pipeline::FetchPacket;

class FetchStage {
    public:
        void Evaluate (
            PipelineLatch<uint32_t>     &pc, 
            PipelineLatch<FetchPacket>  &if_id,
            bool                        stall,
            const mem::MemoryInterface  &mem
        );
};

class DecodeStage {
    public:
        void Evaluate (
            const PipelineLatch<FetchPacket>  &if_id,
            PipelineLatch<DecodePacket> &id_ex,
            core::RegisterFile          &rf
        );
};

class ExecuteStage {
    public:
        void Evaluate (
            const PipelineLatch<DecodePacket>  &id_ex,
            PipelineLatch<ExecutePacket> &ex_mem
        );
    };
    
class MemoryStage {
    public:
    void Evaluate (
        const PipelineLatch<ExecutePacket> &ex_mem,
        PipelineLatch<MemoryPacket>  &mem_wb
    );
};
    
class WritebackStage {
    public:
    void Evaluate (
        const PipelineLatch<MemoryPacket>  &mem_wb,
        core::ArchState              &arch
    );
};





} // namespace


#endif // NF_PIPELINE_STAGES_HPP_