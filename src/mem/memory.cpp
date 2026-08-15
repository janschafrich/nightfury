#include "nf/mem/memory.hpp"

#include <algorithm>
#include <sstream>

namespace nf::mem {

namespace {

constexpr uint32_t SizeBytes(ElementSize size) {
    switch (size) {
        case ElementSize::kByte:     return 1;
        case ElementSize::kHalfword: return 2;
        case ElementSize::kWord:     return 4;
    }
    return 0;  // unreachable: ElementSize has no other enumerators
}

std::string BuildMessage(uint32_t addr, ElementSize size) {
    std::ostringstream oss;
    oss << "access fault: addr=0x" << std::hex << addr
        << " width=" << std::dec << SizeBytes(size) << "B out of range";
    return oss.str();
}

}  // namespace

AccessFault::AccessFault(uint32_t addr, ElementSize size)
    : std::runtime_error(BuildMessage(addr, size)), addr_(addr), size_(size) {}

Memory::Memory(uint32_t base_addr, size_t size_bytes)
    : base_(base_addr), mem_(size_bytes) {}

size_t Memory::Translate(uint32_t addr, ElementSize size) const {
    if (addr < base_) {
        throw AccessFault(addr, size);
    }

    const size_t index = static_cast<size_t>(addr) - base_;
    const size_t width = SizeBytes(size);
    if (index + width > mem_.size()) {
        throw AccessFault(addr, size);
    }

    return index;
}

uint32_t Memory::Load(ElementSize size, uint32_t addr) const {
    const size_t index = Translate(addr, size);
    const uint32_t width = SizeBytes(size);

    uint32_t val = 0;
    for (uint32_t i = 0; i < width; ++i) {
        val |= static_cast<uint32_t>(mem_[index + i]) << (8 * i);
    }
    return val;
}

void Memory::Store(ElementSize size, uint32_t addr, uint32_t val) {
    const size_t index = Translate(addr, size);
    const uint32_t width = SizeBytes(size);

    for (uint32_t i = 0; i < width; ++i) {
        mem_[index + i] = static_cast<uint8_t>((val >> (8 * i)) & 0xFF);
    }
}

void Memory::WriteBlob(uint32_t addr, std::span<const uint8_t> data) {
    // Used to initialize the memory before simulating the CPU's execution
    // Never called as a result of a decoded instruction.
    if (addr < base_) {
        throw AccessFault(addr, ElementSize::kByte);
    }
    const size_t index = static_cast<size_t>(addr) - base_;
    if (index + data.size() > mem_.size()) {
        throw AccessFault(addr, ElementSize::kByte);
    }
    std::copy(data.begin(), data.end(), mem_.begin() + static_cast<std::ptrdiff_t>(index));
}

}  // namespace nf::mem
