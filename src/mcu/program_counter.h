#pragma once

#include <cstddef>
#include <cstdint>

class ProgramCounter {
public:
    uint32_t value() const { return pc; }
    void reset() { pc = 0; }
    void jump(uint32_t address) { pc = address; }
    void advance(uint32_t amount = 1) { pc += amount; }

private:
    uint32_t pc = 0;
};
