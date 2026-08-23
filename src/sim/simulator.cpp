#include "nf/sim/simulator.hpp"

#include <stdexcept>

namespace nf::sim {

using nf::core::Interpreter;
using nf::core::StepResult;
using nf::loader::LoadElfFile;
using nf::mem::ElementSize;

namespace {

// Loads `elf_path` into `memory` exactly once and unwraps the two ways that
// can fail into an exception, since Simulator's constructor has no
// std::expected-shaped return channel to propagate them through -- same
// "fail fast on a setup bug" rationale as mem::AccessFault.
ElfImage RequireLoad(const std::string &elf_path, Memory &memory) {
    auto image = LoadElfFile(elf_path, memory);
    if (!image) {
        throw std::runtime_error(image.error().message);
    }
    if (!image->tohost_addr) {
        throw std::runtime_error("'" + elf_path + "' has no 'tohost' symbol");
    }
    return *image;
}

}  // namespace

std::string_view Simulator::Version() { return "0.1.0"; }

Simulator::Simulator(const std::string &elf_path, uint32_t mem_base_addr, size_t mem_size_bytes)
    // member initialization list
    : _memory(mem_base_addr, mem_size_bytes),
      _image(RequireLoad(elf_path, _memory)),
      _interpreter(_memory, _image.entry_pc, *_image.tohost_addr),
      _tohost_addr(*_image.tohost_addr) {}  // constructor body is empty

SimResult Simulator::Run() {
    StepResult result;
    do {
        result = _interpreter.ProcessInstruction();
    } while (result != StepResult::kHalted);

    const uint32_t tohost_value = _memory.Load(ElementSize::kWord, _tohost_addr);

    return SimResult{
        .passed         = (tohost_value == 1),
        .tohost_value   = tohost_value
    };
}

}  // namespace nf::sim
