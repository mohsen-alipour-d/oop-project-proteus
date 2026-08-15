#include "flash_memory.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

FlashMemory::FlashMemory(size_t size) : bytes(size, 0xFF) {}

void FlashMemory::clear() {
    std::fill(bytes.begin(), bytes.end(), 0xFF);
    highestWritten = 0;
}

uint8_t FlashMemory::read(size_t address) const {
    if (address >= bytes.size())
        throw std::out_of_range("MCU flash address out of range");
    return bytes[address];
}

void FlashMemory::write(size_t address, uint8_t value) {
    if (address >= bytes.size())
        throw std::out_of_range("MCU flash address out of range");
    bytes[address] = value;
    if (address + 1 > highestWritten)
        highestWritten = address + 1;
}

std::string FlashMemory::exportHex() const {
    if (highestWritten == 0)
        return "-";
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0');
    for (size_t i = 0; i < highestWritten; ++i)
        out << std::setw(2) << static_cast<int>(bytes[i]);
    return out.str();
}

bool FlashMemory::importHex(const std::string& hex) {
    clear();
    if (hex == "-" || hex.empty())
        return true;
    if (hex.size() % 2 != 0 || hex.size() / 2 > bytes.size())
        return false;

    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return -1;
    };

    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = nibble(hex[i]);
        int lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) {
            clear();
            return false;
        }
        write(i / 2, static_cast<uint8_t>((hi << 4) | lo));
    }
    return true;
}
