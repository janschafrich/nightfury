#include <catch2/catch_test_macros.hpp>

#include "nf/core/register_file.hpp"

using nf::core::RegisterFile;

TEST_CASE("Read back what was written") {
    for (auto i = 1; i < 32; i++) {
        auto reg = i;
        auto val = 66 + i;
        RegisterFile reg_file;
        reg_file.Write(reg, val);
        auto rs = reg_file.Read(reg);
        CAPTURE(i); CHECK(rs == val);
    }
}
