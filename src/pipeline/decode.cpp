#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/core/register_file.hpp"
#include "nf/isa/decoder.hpp"


namespace nf::pipeline {
    // decode instruction
    // read register file
DecodeStage::Evaluate(
    const pipeline::PipelineLatch<FetchPacket>  &if_id,
    pipeline::PipelineLatch<DecodePacket>       &id_ex,
    const core::RegisterFile                    &regfile,
    const bool                                  csm_ready
) {
    if (!csm_ready) {
        if_id.Stall();
        return;
    }

    if (!if_id.Valid()) {
        id_ex.SetNop();
        return;
    }

    isa::Decoder decoder;
    inst_dec = decoder.Decode(if_id.raw_word);

    DecodePacket pkt;
    pkt.pc = if_id.pc;
    pkt.decoded_inst = inst_dec;
    pkt.rs1_val = regfile.Read(inst_dec.rs1);
    pkt.rs2_val = regfile.Read(inst_dec.rs1);

    id_ex.SetNext(pkt);

    return;
}

} // namespace