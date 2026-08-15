#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class InternalRAM {
public:
    explicit InternalRAM(size_t size = 256) : bytes(size, 0) {}

    void clear();
    uint8_t read(size_t address) const;
    void write(size_t address, uint8_t value);
    size_t size() const { return bytes.size(); }

private:
    std::vector<uint8_t> bytes;
};
