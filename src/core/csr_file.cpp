#include "nf/core/csr_file.hpp"

namespace nf::core {

namespace {
constexpr uint16_t kMtvec = 0x305;
constexpr uint16_t kMepc = 0x341;
constexpr uint16_t kMcause = 0x342;
constexpr uint16_t kMhartid = 0xF14;
}  // namespace

uint32_t CsrFile::Read(uint16_t addr) const {
    switch (addr) {
        case kMtvec: return mtvec_;
        case kMepc: return mepc_;
        case kMcause: return mcause_;
        case kMhartid: return 0;  // single-hart machine, always hart 0
        default: return 0;       // unmodeled CSR: no backing state
    }
}

void CsrFile::Write(uint16_t addr, uint32_t val) {
    switch (addr) {
        case kMtvec: mtvec_ = val; break;
        case kMepc: mepc_ = val; break;
        case kMcause: mcause_ = val; break;
        default: break;  // unmodeled CSR: write is a no-op, see header comment
    }
}

}  // namespace nf::core
