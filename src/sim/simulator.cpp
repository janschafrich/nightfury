#include "nf/sim/simulator.hpp"

namespace nf::sim {

std::string_view Simulator::Version() { return "0.1.0"; }
// instantiate memory and interperter
// Simulator::Run() calls ProcessInstruction repeatedly
// stop simulation on kHalted
// read test result 
//        auto test_result = memory_.Load(ElementSize::kWord, kTohost);
// 

}  // namespace nf::sim
