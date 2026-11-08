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
enum class WbAction : uint8_t {
    kNone, kEcall, kEbreak, kMret, kWrite
};

struct CsrPayload {
    uint32_t val = 0;                   // meaningful for kWrite
    uint16_t addr = 0;                  // 12 bit address space, meaningful for kWrite
    WbAction action = WbAction::kNone;  // not a CSR commit

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
    CsrPayload csr;
    bool operator==(const DecodePacket&) const = default;
};

struct ExecutePacket {
    uint32_t    pc;
    uint32_t    alu_result;         // contains results from ALU ops, target address or ld/st address
    uint32_t    store_val;          // for mem stage
    uint8_t     rd;                 // WB writes if rd != 0
    ElementSize mem_size;           // for mem stage
    bool        is_control;         // jump or branch, ecall, mret
    bool        is_load;            // for mem stage
    bool        is_store;           // for mem stage
    bool        sign_extend;        // 0 unsigned, 1 signed
    CsrPayload  csr;                // for wb stage

    bool operator==(const ExecutePacket&) const = default;
};

struct MemoryPacket {
    uint32_t    pc;             // for mepc
    uint32_t    wb_data;        // either ALU result or load data
    uint8_t     rd;             // if rd = 0 no write back needed
    CsrPayload  csr;            // passthrough for wb stage
    bool        halt = false;   // Halt simulation, e.g. on detection of store to to_host
    bool operator==(const MemoryPacket&) const = default;
};


}


#endif // NF_PIPELINE_PAYLOADS_HPP_