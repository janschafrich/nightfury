#ifndef NF_CORE_ARCH_STATE_HPP_
#define NF_CORE_ARCH_STATE_HPP_

#include "nf/core/register_file.hpp"
#include "nf/core/csr_file.hpp"

namespace nf::core {

// Architectural state shared by every CPU model (Gem5 ThreadContext
// pattern). 
// Owned by Simulator, consumed by reference. PC deliberately
// excluded: each model owns its own fetch PC -- it is model-local.
struct ArchState {
    RegisterFile regs;
    CsrFile csrs;

    bool operator==(const ArchState &) const = default;
};

}  // namespace nf::core

#endif // NF_CORE_ARCH_STATE_HPP_