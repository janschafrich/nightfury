#ifndef NF_PIPELINE_PAYLOADS_HPP_
#define NF_PIPELINE_PAYLOADS_HPP_

#include <cstdint>
#include "nf/isa/decoder.hpp"
#include "nf/mem/memory.hpp"


namespace nf::pipeline {
    
using nf::mem::ElementSize;
using nf::isa::DecodedInstruction;

// Packets define the outputs of the given state
// Packets contain only what the downstream stage needs
// They thereby define at which point in the pipeline a result is produced
// Bubbles are represented by invalid packets.

enum class CsrKind : uint8_t { kNone, kWrite, kEcall };

struct CsrPayload {
    uint32_t new_value = 0;         // meaningful for kWrite
    uint16_t addr = 0;              // 12 bit address space, meaningful for kWrite
    CsrKind  kind = CsrKind::kNone;  // not a CSR commit

    bool operator==(const CsrPayload &) const = default;

};

struct Redirect {
    uint32_t pc = 0;
    bool valid  = false;
};


struct FetchPacket {
    uint32_t pc;      
    uint32_t raw_word;

    bool operator==(const FetchPacket&) const = default;
};

struct DecodePacket {
    uint32_t pc;
    DecodedInstruction decoded_inst;
    uint32_t rs1_val;
    uint32_t rs2_val;

    bool operator==(const DecodePacket&) const = default;
};

struct ExecutePacket {
    uint32_t    pc;
    uint32_t    rs1_val;           // passthrough for mem stage
    uint32_t    rs2_val;           // passthrough for mem stage
    uint32_t    alu_result;        // contains results from ALU ops, and target address
    uint32_t    store_val;
    uint32_t    pc_next;           // branch target or pc+4
    uint8_t     rd;                // WB writes if rd != 0
    ElementSize mem_size;
    bool        is_control;        // jump or branch, ecall, mret
    bool        is_load;
    bool        is_store;
    bool        sign_extend;        // 0 unsigned, 1 signed
    CsrPayload  csr;             // passthrough for wb stage

    bool operator==(const ExecutePacket&) const = default;
};

struct MemoryPacket {
    uint32_t    pc;
    uint32_t    wb_data;       // either ALU result or load data
    uint8_t     rd;            // if rd = 0 no write back needed
    CsrPayload  csr;         // passthrough for wb stage
    
    bool operator==(const MemoryPacket&) const = default;
};


}


#endif // NF_PIPELINE_PAYLOADS_HPP_