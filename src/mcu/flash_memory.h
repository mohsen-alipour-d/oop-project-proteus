#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class FlashMemory {
public:
    explicit FlashMemory(size_t size = 65536);

    void clear();
    uint8_t read(size_t address) const;
    void write(size_t address, uint8_t value);
    size_t size() const { return bytes.size(); }
    size_t usedSize() const { return highestWritten; }

    std::string exportHex() const;
    bool importHex(const std::string& hex);

private:
    std::vector<uint8_t> bytes;
    size_t highestWritten = 0;
};
