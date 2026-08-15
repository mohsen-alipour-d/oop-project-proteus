#include "mcu_port.h"

#include <stdexcept>

#include "../components/digital/logic_level.h"

void MCUPort::bind(const std::array<Pin*, 8>& portPins) {
    pins = portPins;
    updatePins();
}

void MCUPort::reset() {
    outputLatch = 0;
    outputDirectionMask = 0xFF;
    updatePins();
}

void MCUPort::setDirectionMask(uint8_t outputMask) {
    outputDirectionMask = outputMask;
    updatePins();
}

void MCUPort::write(uint8_t value) {
    outputLatch = value;
    updatePins();
}

uint8_t MCUPort::read() const {
    uint8_t value = 0;
    for (int bit = 0; bit < 8; ++bit) {
        Pin* pin = pins[bit];
        if (!pin)
            continue;
        if ((outputDirectionMask >> bit) & 1u) {
            if ((outputLatch >> bit) & 1u)
                value |= static_cast<uint8_t>(1u << bit);
        } else {
            LogicLevel level = voltageToLogic(pin->voltage);
            if (level == HIGH)
                value |= static_cast<uint8_t>(1u << bit);
        }
    }
    return value;
}

void MCUPort::setBit(int bit) {
    if (bit < 0 || bit > 7)
        throw std::out_of_range("MCU port bit out of range");
    outputLatch |= static_cast<uint8_t>(1u << bit);
    updatePins();
}

void MCUPort::clearBit(int bit) {
    if (bit < 0 || bit > 7)
        throw std::out_of_range("MCU port bit out of range");
    outputLatch &= static_cast<uint8_t>(~(1u << bit));
    updatePins();
}

void MCUPort::updatePins() {
    for (int bit = 0; bit < 8; ++bit) {
        Pin* pin = pins[bit];
        if (!pin)
            continue;
        pin->setDirection(PinDirection::Bidirectional);
        const bool output = ((outputDirectionMask >> bit) & 1u) != 0;
        pin->setDriving(output);
        if (output)
            pin->voltage = ((outputLatch >> bit) & 1u) ? 5.0 : 0.0;
    }
}
