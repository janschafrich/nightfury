#include <catch2/catch_test_macros.hpp>

#include "nf/core/arch_state.hpp"
#include "nf/core/register_file.hpp"
#include "nf/core/csr_file.hpp"

using nf::core::ArchState;

TEST_CASE("Fresh ArchState is all zero") {

    ArchState as;

    for (int i = 0; i < 32; i++) {
        CHECK(as.regs.Read(i) == 0);
    }
    
    CHECK(as.csrs.mtvec() == 0);
    CHECK(as.csrs.mepc() == 0);
}

TEST_CASE("operator== compares by value, equal fresh, unequal after write") {
    ArchState as1;
    ArchState as2;

    // Should be initialized to the same values
    CHECK(as1 == as2);

    as1.regs.Write(1, 42);
    CHECK(as1 != as2);
    
    as2.regs.Write(1, 42);
    CHECK(as1 == as2);
}

TEST_CASE("Copy by value: copying arch state snapshots its state") {
    ArchState as1;
    ArchState as1_copy = as1;

    CHECK(as1 == as1_copy);

    as1.regs.Write(1, 42);
    CHECK(as1 != as1_copy);
}


