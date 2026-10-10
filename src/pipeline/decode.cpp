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
    core::RegisterFile                      &rf
) {

    isa::Decoder decoder;

    inst_dec = decoder.Decode(if_id.raw_word);

    id_ex = {
        .pc = if_id.pc
        .decoded_inst = inst_dec,
        .rs1_val = 0,
        .rs2_val = 0,
        .valid = false
    }

}

} // namespace