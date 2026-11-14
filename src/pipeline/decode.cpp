#include "nf/pipeline/stages.hpp"
#include "nf/pipeline/pipeline.hpp"
#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"
#include "nf/core/register_file.hpp"
#include "nf/isa/decoder.hpp"


namespace nf::pipeline {

using nf::isa::Mnemonic;
    // decode instruction
    // read register file
void DecodeStage::Evaluate(
    pipeline::PipelineLatch<FetchPacket>    &if_id,
    pipeline::PipelineLatch<DecodePacket>   &id_ex,
    const core::RegisterFile                &regfile,
    const bool                              csm_ready
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
    DecodedInstruction inst_dec = decoder.Decode(if_id.Q().raw_word);

    DecodePacket pkt;
    CsrPayload   csr;
    
    // *i variants take a 5-bit zero-extended immediate straight off
    // the rs1 field -- it's not a register number for these (see
    // decoder.cpp); register variants use rs1_val like any other op.
    const bool is_immediate_form =
    inst_dec.mnemonic == Mnemonic::kCsrrwi ||
    inst_dec.mnemonic == Mnemonic::kCsrrsi ||
    inst_dec.mnemonic == Mnemonic::kCsrrci;
    uint32_t rs1_val = regfile.Read(inst_dec.rs1);
    csr.val     = is_immediate_form ? inst_dec.rs1 : rs1_val;
    csr.action  = nf::pipeline::ToWbAction(inst_dec.mnemonic);
    csr.addr    = static_cast<uint16_t>(inst_dec.imm);

    pkt.pc              = if_id.Q().pc;
    pkt.decoded_inst    = inst_dec;
    pkt.rs1_val         = rs1_val;
    pkt.rs2_val         = regfile.Read(inst_dec.rs2);
    pkt.csr             = csr;

    id_ex.Set(pkt);

    return;
}

} // namespace