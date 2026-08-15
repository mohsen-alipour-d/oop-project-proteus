#include "register_file.h"

void RegisterFile::reset() {
    a = 0;
    general.fill(0);
}

uint8_t RegisterFile::get(int index) const {
    if (index == 0)
        return a;
    if (index >= 1 && index <= 8)
        return general[index - 1];
    throw std::out_of_range("MCU register index out of range");
}

void RegisterFile::set(int index, uint8_t value) {
    if (index == 0) {
        a = value;
        return;
    }
    if (index >= 1 && index <= 8) {
        general[index - 1] = value;
        return;
    }
    throw std::out_of_range("MCU register index out of range");
}
