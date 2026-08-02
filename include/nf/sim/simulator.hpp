#ifndef NF_SIM_SIMULATOR_HPP_
#define NF_SIM_SIMULATOR_HPP_

#include <string_view>

namespace nf::sim {

// Top-level orchestrator: will own memory + pipeline, run the cycle loop,
// and collect statistics. Real behavior lands in later milestones.
class Simulator {
public:
    static std::string_view Version();
};

}  // namespace nf::sim

#endif  // NF_SIM_SIMULATOR_HPP_
