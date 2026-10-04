#ifndef NF_CORE_INTERPRETER_HPP_
#define NF_CORE_INTERPRETER_HPP_

#include <cstdint>

#include "nf/isa/decoder.hpp"
#include "nf/core/register_file.hpp"
#include "nf/core/csr_file.hpp"
#include "nf/core/arch_state.hpp"
#include "nf/core/alu.hpp"
#include "nf/mem/memory.hpp"


namespace nf::core {

enum class StepResult {kOk, kHalted};

class Interpreter {
public:
    Interpreter(
        mem::MemoryInterface &memory, 
        core::ArchState &arch, 
        uint32_t reset_pc, 
        uint32_t tohost_addr); // constructor
    StepResult ProcessInstruction();  
    
private:
    RegisterFile            &regfile_;          // Simulator owned, shared with pipeline
    CsrFile                 &csrs_;             // Minimal M-mode CSR state (see csr_file.hpp), shared with pipeline
    mem::MemoryInterface    &memory_;      // Share between functional core and pipeline,  from Memory to MemoryInterface
    isa::DecodedInstruction inst_;   // Current instruction
    uint32_t                pc_;               // Current PC for branch target calculation
    uint32_t                tohost_addr_;
};


}  // namespace nf::core

#endif  // NF_CORE_INTERPRETER_HPP_
