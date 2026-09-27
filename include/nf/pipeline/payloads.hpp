#ifndef NF_PIPELINE_PAYLOADS_HPP_
#define NF_PIPELINE_PAYLOADS_HPP_

#include <cstdint>
#include "nf/isa/decoder.hpp"
#include "nf/mem/memory.hpp"

namespace nf::pipeline {

// Packets define the outputs of the given state
// Packets contain only what the downstream stage needs
// They thereby define at which point in the pipeline a result is produced
// Bubbles are represented by invalid packets.

struct FetchPacket {
    uint32_t pc;      
    uint32_t raw_word;
    bool     valid;
};

struct DecodePacket {
    uint32_t pc;
    nf::isa::DecodedInstruction decoded_inst;
    uint32_t rs1_val;
    uint32_t rs2_val;
    bool     valid;
};

struct ExecutePacket {
    uint32_t pc;
    uint32_t rs1_val;           // for mem stage
    uint32_t rs2_val;           // for mem stage
    uint32_t alu_result;        // contains results from ALU ops, and target address
    uint32_t store_val;
    uint32_t branch_target;
    uint8_t  rd;                // WB writes if rd != 0
    nf::mem::ElementSize size;
    bool     branch_taken;
    bool     is_load;            // 0 load, 1 store
    bool     is_store;           // 0 load, 1 store
    bool     sign_extend;        // 0 unsigned, 1 signed
    bool     valid;
};

struct MemoryPacket {
    uint32_t pc;
    uint8_t  rd;
    bool     valid;
    // if rd = 0 no write back needed
};


}


#endif // NF_PIPELINE_PAYLOADS_HPP_