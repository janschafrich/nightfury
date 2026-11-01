#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/core/register_file.hpp"
#include "nf/core/csr_file.hpp"

namespace nf::pipeline {
    // regfile / CSR write
    // ecall effects

using nf::pipeline::WbAction;

Redirect WritebackStage::Evaluate(
    PipelineLatch<MemoryPacket>   &mem_wb,
    core::RegisterFile            &regfile,
    core::CsrFile                 &csrs
) {
    Redirect redirect{};

    if (!mem_wb.Valid()) {
        mem_wb.SetNop();
        return {redirect.valid = false};
    }

    regfile.Write(mem_wb.Q().rd, mem_wb.Q().wb_data);

    switch (mem_wb.Q().csr.action) {
        // case (WbAction::kWrite):
        //     csrs.Write(mem_wb.Q().csr.addr, mem_wb.Q().csr.new_value);
        //     break;
        case (WbAction::kEcall):
            // Write mstatus not modeled.
            csrs.set_mepc(mem_wb.Q().pc);
            csrs.set_mcause(nf::core::kCauseMachineEcall);
            redirect.valid  = true;
            redirect.pc     = csrs.mtvec();
            break;
        case (WbAction::kEbreak):
            // Write mstatus not modeled.
            csrs.set_mepc(mem_wb.Q().pc);
            redirect.valid  = true;
            redirect.pc     = csrs.mtvec();
            break;
        case (WbAction::kMret):
            // Write mstatus, switch privilege not modeled.
            redirect.valid  = true;
            redirect.pc     = csrs.mepc();
            break;
        default: break;
    }
    
    return redirect;
}

} // namespace
