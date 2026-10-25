#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/core/register_file.hpp"
#include "nf/core/csr_file.hpp"

namespace nf::pipeline {
    // regfile / CSR write
    // ecall effects

WritebackStage::Evaluate(
    const PipelineLatch<MemoryPacket>   &mem_wb,
    core::RegisterFile                  &regfile,
    core::CsrFile                       &csrs
)

    if (!mem_wb.Valid()) {
        mem_wb.SetNop();
        return;
    }

    Redirect redirect{};


    regfile.Write(mem_wb.rd, mem_wb.wb_data);

    // ??
    // csr_file()
    return redirect;

} // namespace
