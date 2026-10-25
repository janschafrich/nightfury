#include <catch2/catch_test_macros.hpp>

#include "nf/pipeline/latch.hpp"
#include "nf/pipeline/payloads.hpp"

using nf::pipeline::PipelineLatch;
using nf::pipeline::FetchPacket;
using nf::pipeline::DecodePacket;
using nf::pipeline::ExecutePacket;
using nf::pipeline::MemoryPacket;

TEST_CASE("Latch hides next value till Advance") {
    FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013};
    FetchPacket second {.pc = 0x1004, .raw_word = 0x00000013};

    PipelineLatch<FetchPacket> latch;

    // New value is not visible before Advance
    latch.Set(first);   // propagate
    CHECK(!latch.Valid());
    
    // New fetch packet is visible after Advance
    latch.Advance(); // clock edge
    CHECK(latch.Valid());

    // Producer wrote new q_ (next cycle), consumer still sees d_ of this cycle
    latch.Set(second);
    CHECK(latch.Q() == first);

    latch.Advance();
    CHECK(latch.Q() == second);
}

TEST_CASE("Fresh latch holds a bubble") {
    PipelineLatch<FetchPacket> latch;
    CHECK(!latch.Valid());
}

TEST_CASE("Back to back Advances one value per cycle") {
    FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013};
    FetchPacket second {.pc = 0x1004, .raw_word = 0x00000013};
    
    PipelineLatch<FetchPacket> latch;

    latch.Set(first);
    latch.Advance();
    CHECK(latch.Q().pc == 0x1000);
    
    latch.Set(second);
    latch.Advance();
    CHECK(latch.Q().pc == 0x1004);
    latch.Advance();
    CHECK(latch.Q().pc == 0x1004);

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

    CHECK(!latch_fe.Valid());
    CHECK(!latch_de.Valid());
    CHECK(!latch_ex.Valid());
    CHECK(!latch_mem.Valid());

}