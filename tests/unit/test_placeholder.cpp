#include <catch2/catch_test_macros.hpp>

#include "nf/sim/simulator.hpp"

TEST_CASE("Simulator reports a version string", "[sim]") {
    REQUIRE(nf::sim::Simulator::version() == "0.1.0");
}
