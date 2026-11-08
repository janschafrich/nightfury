#include <utility>

#include "nf/core/csr_update.hpp"
#include "nf/isa/decoder.hpp"

namespace nf::core {

using nf::isa::Mnemonic;

uint32_t CsrUpdate(Mnemonic m, uint32_t old_val, uint32_t operand) {

    uint32_t new_val = old_val;
    switch (m) {
        // Read Write: swap CSR with integer GPR
        case Mnemonic::kCsrrw: case Mnemonic::kCsrrwi: new_val = operand; break;
        // Set bits according to rs1
        case Mnemonic::kCsrrs: case Mnemonic::kCsrrsi: new_val = old_val | operand; break;
        // Clear bits according to rs1
        case Mnemonic::kCsrrc: case Mnemonic::kCsrrci: new_val = old_val & ~operand; break;

        // A CSR usually has a architecturally defined role, defined semantics and hardware side effects. 
        // Examples of CSRs: mepc, instret, fpsr
        // Use cases for these register are:
        //   1. Application software: counters, timers, floating point
        //   2. OS: during context switch: sstatus, sepc, scause, satp (virtual memory), sie
        // Software uses inline assembly to write to these registers.
        // The 4096 CSR address space is divided between U, M and S.
        // Some regions are read only for U or M. 
        // The compiler does not check RISC-V semantic, just syntax. 
        // It will compile U CSR write to a readonly region.
        // At runtime the CPU checks whether
        //   1. the CSR exists
        //   2. whether the current privilege mode is allowed to access
        //   3. whether a write is permitted
        // Invalid access raise an illegal-instruction exception.
        
        // uArch typically implement this control logic together with CSR file, which performs the atomic R/W.

        default: std::unreachable();
    }

    return new_val;
}

} // namespace