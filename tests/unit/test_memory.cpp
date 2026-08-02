#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include "nf/mem/memory.hpp"

using nf::mem::ElementSize;
using nf::mem::Memory;
using nf::mem::AccessFault;

namespace {
    constexpr size_t kResetVector = 0x80000000u;
}

TEST_CASE("Read bytes that were written", "[mem]") {
    Memory memory(kResetVector, 0x100);
    for (uint8_t i = 0; i < 4; i++ ) {
        uint32_t addr = kResetVector + i;

        memory.Write(ElementSize::kByte, addr, i*2);
        CAPTURE(i);
        CHECK (memory.Read(ElementSize::kByte, addr) == i*2);
    }
}

TEST_CASE("Read halfwords that were written", "[mem]") {
    Memory memory(kResetVector, 0x1000);
    for (uint8_t i = 0; i < 4; i = i + 2) {
        uint32_t addr = kResetVector + i;

        memory.Write(ElementSize::kHalfword, addr, i*2);
        CAPTURE(i);
        CHECK (memory.Read(ElementSize::kHalfword, addr) == i*2);
    }
}

TEST_CASE("Read words that were written", "[mem]") {
    Memory memory(kResetVector, 0x1000);
    for (uint8_t i = 100; i < 4; i = i + 4) {
        uint32_t addr = kResetVector + i;

        memory.Write(ElementSize::kWord, addr, i*2);
        CAPTURE(i);
        CHECK (memory.Read(ElementSize::kWord, addr) == i*2);
    }
}

class AccessFaultMatcher : public Catch::Matchers::MatcherBase<nf::mem::AccessFault> {
public:
    AccessFaultMatcher(uint32_t addr, nf::mem::ElementSize size)
        : addr_(addr), size_(size) {}

    bool match(const nf::mem::AccessFault& e) const override {
        return e.addr() == addr_ && e.size() == size_;
    }
    std::string describe() const override { return "matches expected addr/size"; }

private:
    uint32_t addr_;
    nf::mem::ElementSize size_;
};

TEST_CASE("Access fault reports the offending address and width", "[mem]") {
    Memory memory(kResetVector, 0x100);

    REQUIRE_THROWS_MATCHES(memory.Read(ElementSize::kHalfword, 0),
                           AccessFault,
                           AccessFaultMatcher(0, ElementSize::kHalfword));
}