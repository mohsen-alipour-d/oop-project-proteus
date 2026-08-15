#include "internal_ram.h"

#include <algorithm>
#include <stdexcept>

void InternalRAM::clear() {
    std::fill(bytes.begin(), bytes.end(), 0);
}

uint8_t InternalRAM::read(size_t address) const {
    if (address >= bytes.size())
        throw std::out_of_range("MCU RAM address out of range");
    return bytes[address];
}

void InternalRAM::write(size_t address, uint8_t value) {
    if (address >= bytes.size())
        throw std::out_of_range("MCU RAM address out of range");
    bytes[address] = value;
}
