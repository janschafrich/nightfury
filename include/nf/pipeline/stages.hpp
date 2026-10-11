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
        bool Ready() const { return true; }       // nothing upstream, fetch is always ready
        void Evaluate (
            PipelineLatch<uint32_t>     &pc, 
            PipelineLatch<FetchPacket>  &if_id,
            bool                        csm_ready,
            const Redirect               &redirect,
            const mem::MemoryInterface  &mem
        );
};

class DecodeStage {
    public:
        bool Ready() const { return true; }     // multi cycle EX will require RAW hazard handling
        void Evaluate (
            const PipelineLatch<FetchPacket>    &if_id,
            PipelineLatch<DecodePacket>         &id_ex,
            bool                                csm_ready,
            core::RegisterFile                  &rf
        );
};

class ExecuteStage {
    public:
        bool Ready() const { return true; }
        void Evaluate (
            const PipelineLatch<DecodePacket>   &id_ex,
            PipelineLatch<ExecutePacket>        &ex_mem,
            bool                                csm_ready
        );
    };
    
class MemoryStage {
    public:
        bool Ready() const { return true;} // replaced by future miss FSM
        void Evaluate (
            const PipelineLatch<ExecutePacket>  &ex_mem,
            PipelineLatch<MemoryPacket>         &mem_wb,
            bool                                csm_ready
        );
};
    
class WritebackStage {
    public:
        bool Ready() const { return true; }
        void Evaluate (
            const PipelineLatch<MemoryPacket>   &mem_wb,
            core::ArchState                     &arch
        );
};





} // namespace


#endif // NF_PIPELINE_STAGES_HPP_