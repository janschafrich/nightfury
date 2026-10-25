#include <catch2/catch_test_macros.hpp>

#include "nf/pipeline/payloads.hpp"
#include "nf/pipeline/latch.hpp"

using nf::pipeline::PipelineLatch;
using nf::pipeline::FetchPacket;



TEST_CASE("Latch is initialized to be non valid") {
    PipelineLatch<FetchPacket> latch;

    CHECK(!latch.Valid());
}

TEST_CASE("Latch is initialized to be ready to accept a payload") {
    PipelineLatch<FetchPacket> latch;

    CHECK(latch.Ready());
}

TEST_CASE("Latch hides data until commit.") {
    FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013};
    FetchPacket second {.pc = 0x1004, .raw_word = 0x00000013};
    PipelineLatch<FetchPacket> latch;

    latch.Set(first);
    CHECK(latch.Q() != first);
    
    latch.Advance();
    CHECK(latch.Q() == first);

}

TEST_CASE("A committed nop reads as invalid data.") {
    FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013};
    PipelineLatch<FetchPacket> latch;

    latch.Set(first);
    latch.Advance();
    CHECK(latch.Valid());

    latch.SetNop();
    latch.Advance();
    CHECK(!latch.Valid());
}

TEST_CASE("A squash always wins over new data data.") {
    FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013};
    FetchPacket second {.pc = 0x1004, .raw_word = 0x00000013};

    PipelineLatch<FetchPacket> latch;

    latch.Set(first);
    latch.Advance();
    CHECK(latch.Q() == first);
    
    latch.Squash();
    latch.Set(second);
    CHECK(latch.Q() != second);

}

TEST_CASE("After a squash the latch must be ready to accept new data.") {
    FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013};
    PipelineLatch<FetchPacket> latch;

    latch.Squash();
    latch.Set(first);
    latch.Advance();
    CHECK(latch.Ready());
    CHECK(!latch.Valid());

}

// Skid Buffer Functionality --------------------------------------------------

// TEST_CASE("If consumer requests stall, the current Q must be kept stable") {
//     FetchPacket first  {.pc = 0x1000, .raw_word = 0x00000013};
//     FetchPacket second {.pc = 0x1004, .raw_word = 0x00000013};
//     PipelineLatch<FetchPacket> latch;

//     latch.Set(first);
//     latch.Advance();
//     CHECK(latch.Q() == first);
    
//     latch.Stall();
//     latch.Set(second);
//     latch.Advance();
//     CHECK(latch.Q() == first);
// }
