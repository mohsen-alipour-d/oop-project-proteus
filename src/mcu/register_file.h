#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>

class RegisterFile {
public:
    void reset();

    uint8_t accumulator() const { return a; }
    void setAccumulator(uint8_t value) { a = value; }

    uint8_t get(int index) const;
    void set(int index, uint8_t value);

private:
    uint8_t a = 0;
    std::array<uint8_t, 8> general{};
};
