#include <iostream>

#include "nf/sim/simulator.hpp"

constexpr uint32_t kMemBase = 0x80000000u;
constexpr uint32_t kMemSize = 2 << 20;   // 2 MiByte

int main(int argc, char *argv[]) {
    // std::cout << "nf-functional (nightfury " << nf::sim::Simulator::Version()
    //           << ") -- functional-only interpreter, not yet implemented\n";
    // return 0;
    if (argc != 2) {
        std::cerr << "Expected a path to an elf to execute.";
        return 2;
    }

    try {
        // user supplied arguments start at 1, not 0
        nf::sim::Simulator sim(argv[1], kMemBase, kMemSize);
        nf::sim::SimResult result = sim.Run();
        if (result.passed) {
            std::cout << "PASS \n";
            return 0;
        }
        // riscv-tests fail encoding: (TESTNUM << 1) | 1 
        std::cout << "FAIL (test case " << (result.tohost_value >> 1) << ")\n";
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "nf-functional: " << e.what() << '\n';        
    }
}
