#pragma once

#include <string_view>

namespace nf::sim {

// Top-level orchestrator: will own memory + pipeline, run the cycle loop,
// and collect statistics. Real behavior lands in later milestones.
class Simulator {
public:
    static std::string_view version();
};

}  // namespace nf::sim
