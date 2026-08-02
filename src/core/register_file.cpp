#include "nf/core/register_file.hpp"

namespace nf::core {

// Skip out of bounds checks, 
// Test decoder to guarantee in bound accesses

void RegisterFile::Write(uint8_t rd, uint32_t val) {
    if (rd != 0) rf_[rd] = val;
}

uint32_t RegisterFile::Read(uint8_t rs) const {
    return rs == 0 ? 0 : rf_[rs];
}

}  // namespace nf::core
