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
using nf::pipeline::Redirect;

class FetchStage {
    public:
        bool Ready() const { return true; }       // nothing upstream, fetch is always ready
        void Evaluate (
            PipelineLatch<uint32_t>     &pc,        
            PipelineLatch<FetchPacket>  &if_id,
            const mem::MemoryInterface  &mem,
            const Redirect              redirect,
            const bool                  csm_ready
        );
};

class DecodeStage {
    public:
        bool Ready() const { return true; }     // multi cycle EX will require RAW hazard handling
        void Evaluate (
            PipelineLatch<FetchPacket>    &if_id,
            PipelineLatch<DecodePacket>   &id_ex,
            const core::RegisterFile      &regfile,
            const bool                    csm_ready
        );
};

class ExecuteStage {
    public:
        bool Ready() const { return true; }
        Redirect Evaluate (
            PipelineLatch<DecodePacket>     &id_ex,
            PipelineLatch<ExecutePacket>    &ex_mem,
            const core::CsrFile             &csrs,
            const bool                      csm_ready
        );
    };
    
class MemoryStage {
    public:
        bool Ready() const { return true;} // replaced by future miss FSM
        void Evaluate (
            PipelineLatch<ExecutePacket>   &ex_mem,
            PipelineLatch<MemoryPacket>    &mem_wb,
            mem::MemoryInterface           &memory,
            const bool                     csm_ready,
            const uint32_t                 tohost_addr
        );
};
    
class WritebackStage {
    public:
        bool Ready() const { return true; }
        Redirect Evaluate (
            PipelineLatch<MemoryPacket>   &mem_wb,
            core::RegisterFile            &regfile,
            core::CsrFile                 &csrs
        );
};





} // namespace


#endif // NF_PIPELINE_STAGES_HPP_