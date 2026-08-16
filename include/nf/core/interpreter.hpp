#ifndef NF_CORE_INTERPRETER_HPP_
#define NF_CORE_INTERPRETER_HPP_

#include <cstdint>

#include "nf/isa/decoder.hpp"
#include "nf/core/register_file.hpp"
#include "nf/core/csr_file.hpp"
#include "nf/core/alu.hpp"
#include "nf/mem/memory.hpp"


namespace nf::core {

enum class StepResult {kOk, kHalted};

class Interpreter {
public:
    Interpreter(mem::MemoryInterface &memory, uint32_t reset_pc); // constructor
    StepResult ProcessInstruction();    

private:
    RegisterFile regfile_;      // Instantiate here as only 32 words
    CsrFile csrs_;              // Minimal M-mode CSR state (see csr_file.hpp)
    mem::MemoryInterface &memory_;    // Share between functional core and pipeline


    uint32_t pc_;               // Current PC for branch target calculation
    isa::DecodedInstruction inst_;   // Current instruction
};


}  // namespace nf::core

#endif  // NF_CORE_INTERPRETER_HPP_
