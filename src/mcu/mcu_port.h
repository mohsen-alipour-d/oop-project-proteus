#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "../core/pin.h"

class MCUPort {
public:
    explicit MCUPort(std::string name = "") : portName(std::move(name)) {}

    void bind(const std::array<Pin*, 8>& portPins);
    void reset();

    void setDirectionMask(uint8_t outputMask);
    uint8_t directionMask() const { return outputDirectionMask; }

    void write(uint8_t value);
    uint8_t read() const;
    void setBit(int bit);
    void clearBit(int bit);
    uint8_t latch() const { return outputLatch; }

private:
    std::string portName;
    std::array<Pin*, 8> pins{};
    uint8_t outputLatch = 0;
    uint8_t outputDirectionMask = 0xFF;

    void updatePins();
};
