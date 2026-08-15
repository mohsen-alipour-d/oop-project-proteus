#pragma once

#include <string>

class Microcontroller;

class InstructionDecoder {
public:
    bool executeNext(Microcontroller& mcu);
    const std::string& lastError() const { return error; }

private:
    std::string error;
    bool fail(const std::string& message);
};
