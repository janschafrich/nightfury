#ifndef NF_CORE_CSR_UPDATE_HPP 
#define NF_CORE_CSR_UPDATE_HPP

#include "nf/isa/decoder.hpp"

namespace nf::core {

uint32_t CsrUpdate(nf::isa::Mnemonic, uint32_t old_val, uint32_t operand);

} // namespace nf::core


#endif // NF_CORE_CSR_UPDATE_HPP