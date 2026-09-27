#ifndef NF_MEM_MEMORY_HPP_
#define NF_MEM_MEMORY_HPP_

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace nf::mem {

enum class ElementSize : uint8_t { kByte, kHalfword, kWord };

// Extends the low `size` bytes of a zero-extended load result to a full
// word, as the MEM-stage extend mux would. Shared by every consumer of
// MemoryInterface (interpreter now, pipeline MEM stage later), so it lives
// here next to ElementSize instead of in any single consumer. The double
// cast relies on C++20 modulo conversions; before C++20 the narrowing was
// implementation-defined.
constexpr uint32_t SignExtend(uint32_t value, ElementSize size) {
    switch (size) {
        case ElementSize::kByte:
            return static_cast<uint32_t>(static_cast<int8_t>(value));
        case ElementSize::kHalfword:
            return static_cast<uint32_t>(static_cast<int16_t>(value));
        case ElementSize::kWord:
            return value;
    }
    return 0;  // unreachable: ElementSize has no other enumerators
}

// Thrown when a load/store falls outside the backing store's address range.
// Not used for architectural traps (e.g. misaligned-access exceptions) --
// those are the interpreter's concern once it models RV32 exception entry.
// A fault here means the guest address translates outside allocated memory,
// which at this milestone always indicates a simulator or test-program bug.
class AccessFault : public std::runtime_error {
public:
    AccessFault(uint32_t addr, ElementSize size);

    uint32_t addr() const { return addr_; }
    ElementSize size() const { return size_; }

private:
    uint32_t addr_;
    ElementSize size_;
};

// Abstract access protocol. Anything issuing a load/store programs against
// this, not against a concrete backing store -- see rationale below.
class MemoryInterface {
public:
    virtual ~MemoryInterface() = default;

    virtual uint32_t Load(ElementSize size, uint32_t addr) const = 0;
    virtual void     Store(ElementSize size, uint32_t addr, uint32_t val) = 0;
};

// Flat, zero-latency backing store for the functional interpreter
// (Milestone 1). Every access completes immediately -- no timing model.
class Memory final : public MemoryInterface {
public:
    Memory(uint32_t base_addr, size_t size_bytes);

    uint32_t Load(ElementSize size, uint32_t addr) const override;
    void     Store(ElementSize size, uint32_t addr, uint32_t val) override;

    // Host-side bulk write for program loading (ELF segments, HTIF setup).
    // Unlike Store(), this is not a simulated CPU access -- it bypasses the
    // element-width access protocol entirely, so it must stay off the
    // MemoryInterface contract that a future timing/cache model intercepts.
    void WriteBlob(uint32_t addr, std::span<const uint8_t> data);

private:
    size_t Translate(uint32_t addr, ElementSize size) const;  // bounds-checked

    uint32_t base_;
    std::vector<uint8_t> mem_;
};

}  // namespace nf::mem

#endif  // NF_MEM_MEMORY_HPP_
