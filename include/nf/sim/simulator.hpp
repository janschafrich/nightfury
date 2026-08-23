#ifndef NF_SIM_SIMULATOR_HPP_
#define NF_SIM_SIMULATOR_HPP_

#include <cstdint>
#include <string>
#include <string_view>

#include "nf/core/interpreter.hpp"
#include "nf/loader/elf_loader.hpp"
#include "nf/mem/memory.hpp"

namespace nf::sim {

using nf::mem::Memory;
using nf::core::Interpreter;
using nf::loader::ElfImage;

// Outcome of running to completion. riscv-tests' write_tohost convention
// (env/p/riscv_test.h) encodes pass as 1 and fail as (test_number << 1) | 1
// -- tohost_value is the raw payload so a failing test number is always
// recoverable as tohost_value >> 1 without re-deriving it here.
struct SimResult {
    bool passed;
    uint32_t tohost_value;
};

// Top-level orchestrator: will own memory + pipeline, run the cycle loop,
// and collect statistics. Real behavior lands in later milestones.
class Simulator {
public:
    // Loads `elf_path` into a Memory spanning [mem_base_addr, mem_base_addr +
    // mem_size_bytes), then starts the interpreter at the ELF's e_entry.
    // Throws std::runtime_error if the file isn't a well-formed image LoadElf
    // accepts, or if it has no "tohost" symbol -- Milestone 1 only runs
    // riscv-tests-style binaries, and the interpreter's halt detection
    // (interpreter.cpp) needs a concrete address to compare stores against.
    Simulator(const std::string &elf_path, uint32_t mem_base_addr, size_t mem_size_bytes);
    SimResult Run();
    static std::string_view Version();

private:
    // Declaration order is initialization order in C++: _image is resolved
    // once _memory exists (LoadElfFile writes PT_LOAD segments into it) and
    // before _interpreter, which consumes _image's entry_pc/tohost_addr.
    Memory _memory;
    ElfImage _image;
    Interpreter _interpreter;
    uint32_t _tohost_addr;
    // future pipeline?

};

}  // namespace nf::sim

#endif  // NF_SIM_SIMULATOR_HPP_
