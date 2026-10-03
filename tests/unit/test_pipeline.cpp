#include <catch2/catch_test_macros.hpp>

#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"

using nf::pipeline::PipelineLatch;
using nf::pipeline::FetchPacket;
using nf::pipeline::DecodePacket;
using nf::pipeline::ExecutePacket;
using nf::pipeline::MemoryPacket;

TEST_CASE("Latch hides next value till Commit") {
    FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013, .valid=true};
    FetchPacket second {.pc = 0x1004, .raw_word = 0x00000013, .valid=true};

    PipelineLatch<FetchPacket> latch;

    // New value is not visible before Commit
    latch.SetNext(first);   // propagate
    CHECK(!latch.Output().valid);
    
    // New fetch packet is visible after commit
    latch.Commit(); // clock edge
    CHECK(latch.Output().valid);

    // Producer wrote new q_ (next cycle), consumer still sees d_ of this cycle
    latch.SetNext(second);
    CHECK(latch.Output() == first);

    latch.Commit();
    CHECK(latch.Output() == second);
}

TEST_CASE("Fresh latch holds a bubble") {
    PipelineLatch<FetchPacket> latch;
    CHECK(!latch.Output().valid);
}

TEST_CASE("Back to back commits one value per cycle") {
    FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013, .valid=true};
    FetchPacket second {.pc = 0x1004, .raw_word = 0x00000013, .valid=true};
    
    PipelineLatch<FetchPacket> latch;

    latch.SetNext(first);
    latch.Commit();
    CHECK(latch.Output().pc == 0x1000);
    
    latch.SetNext(second);
    latch.Commit();
    CHECK(latch.Output().pc == 0x1004);
    latch.Commit();
    CHECK(latch.Output().pc == 0x1004);

}

TEST_CASE("All packet types are latch compatible") {
    FetchPacket   fetch;
    DecodePacket  decode;
    ExecutePacket execute;
    MemoryPacket  memory;

    PipelineLatch<FetchPacket>      latch_fe;
    PipelineLatch<DecodePacket>     latch_de;
    PipelineLatch<ExecutePacket>    latch_ex;
    PipelineLatch<MemoryPacket>     latch_mem;

    CHECK(!latch_fe.Output().valid);
    CHECK(!latch_de.Output().valid);
    CHECK(!latch_ex.Output().valid);
    CHECK(!latch_mem.Output().valid);

}