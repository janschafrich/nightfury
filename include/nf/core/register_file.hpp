#ifndef NF_CORE_REGISTER_FILE_HPP_
#define NF_CORE_REGISTER_FILE_HPP_

#include <cstdint>
#include <vector>

namespace nf::core {

class RegisterFile {
public:
    // Constructor member init list
    RegisterFile() : rf_(32, 0) {}

    void Write(uint8_t rd, uint32_t val);
    uint32_t Read(uint8_t rs) const;
private:
    std::vector<uint32_t> rf_;
};

}  // namespace nf::core

#endif  // NF_CORE_REGISTER_FILE_HPP_