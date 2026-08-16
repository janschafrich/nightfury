#ifndef NF_CORE_CSR_FILE_HPP_
#define NF_CORE_CSR_FILE_HPP_

#include <cstdint>

namespace nf::core {

// mcause code for an ecall taken while already in M-mode. Nightfury never
// leaves M-mode -- riscv-tests' env/p RVTEST_RV32U/RV32UM `init` macro is
// empty, so the hart never transitions to U/S-mode in the first place --
// so this is the only ecall cause this interpreter will ever produce.
constexpr uint32_t kCauseMachineEcall = 0xb;

// Machine-mode CSR state, scoped to exactly what riscv-tests' bare-metal
// boot sequence (env/p/riscv_test.h) touches: enough for ecall to trap to
// mtvec and mret to return, and to identify as hart 0. Nightfury models a
// single always-M-mode core with no PMP/interrupt/delegation semantics, so
// every other CSR address (mstatus, pmpaddr0/pmpcfg0, satp, medeleg,
// mideleg, mie, ...) is a harmless no-op write / zero read -- confirmed by
// reading riscv_test.h: none of INIT_PMP/INIT_SATP/DELEGATE_NO_TRAPS/
// INIT_RNMI read back what they just wrote, so no-op'ing them is
// observationally identical to modeling them for this test suite.
class CsrFile {
public:
    uint32_t Read(uint16_t addr) const;
    void Write(uint16_t addr, uint32_t val);

    uint32_t mtvec() const { return mtvec_; }
    uint32_t mepc() const { return mepc_; }
    void set_mepc(uint32_t val) { mepc_ = val; }
    void set_mcause(uint32_t val) { mcause_ = val; }

private:
    uint32_t mtvec_ = 0;
    uint32_t mepc_ = 0;
    uint32_t mcause_ = 0;
};

}  // namespace nf::core

#endif  // NF_CORE_CSR_FILE_HPP_
