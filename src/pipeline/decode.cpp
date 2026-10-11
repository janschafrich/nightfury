#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/core/register_file.hpp"
#include "nf/isa/decoder.hpp"


namespace nf::pipeline {
    // decode instruction
    // read register file
DecodeStage::Evaluate(
    pipeline::PipelineLatch<FetchPacket>    &if_id,
    pipeline::PipelineLatch<DecodePacket>   &id_ex,
    core::RegisterFile                      &rf,
    bool                                    csm_ready
) {

    isa::Decoder decoder;

    inst_dec = decoder.Decode(if_id.raw_word);

    DecodePacket pkt;
    pkt.pc = if_id.pc;
    pkt.decoded_inst = inst_dec;
    pkt.rs1_val = 0;    // rf read
    pkt.rs2_val = 0;
    pkt.valid = true;

    id_ex.SetNext(pkt);
}

} // namespace