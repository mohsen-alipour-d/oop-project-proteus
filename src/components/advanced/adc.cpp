#include "adc.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

ADC::ADC(string name, double x, double y, int bitCount, double conversionDelay)
    : Component(name, x, y) {
    if (bitCount < 1 || bitCount > 31)
        throw invalid_argument("ADC bit count must be between 1 and 31");

    this->bitCount = bitCount;
    setConversionDelay(conversionDelay);

    // Analog input and reference pins.
    addPin(-30, 0);   // Vin
    addPin(-15, -20); // Vref+
    addPin(-15, 20);  // Vref-

    // D0 is the least-significant bit. Every output bit is independently
    // wireable, as required by Part 7.1.
    for (int bit = 0; bit < bitCount; ++bit) {
        addPin(30, bit * 10 - (bitCount - 1) * 5);
        pins.back().setDirection(PinDirection::Output);
    }

    applyCode(0);
}

void ADC::setConversionDelay(double delay) {
    if (delay < 0.0)
        throw invalid_argument("ADC conversion delay cannot be negative");
    conversionDelay = delay;
}

uint32_t ADC::calculateCode() const {
    const double vin = pins[VIN_PIN].voltage;
    const double vrefPlus = pins[VREF_PLUS_PIN].voltage;
    const double vrefMinus = pins[VREF_MINUS_PIN].voltage;

    if (!(vrefPlus > vrefMinus))
        return currentCode;

    const uint32_t maxCode = (uint32_t(1) << bitCount) - 1u;

    if (vin <= vrefMinus)
        return 0;
    if (vin >= vrefPlus)
        return maxCode;

    const double ratio = (vin - vrefMinus) / (vrefPlus - vrefMinus);
    const double mapped = ratio * static_cast<double>(maxCode);
    return static_cast<uint32_t>(llround(mapped));
}

void ADC::applyCode(uint32_t code) {
    currentCode = code;
    for (int bit = 0; bit < bitCount; ++bit) {
        const bool high = ((code >> bit) & 1u) != 0;
        pins[outputPinIndex(bit)].voltage = high ? 5.0 : 0.0;
    }
}

void ADC::step(double dt, double simTime) {
    (void)dt;
    const uint32_t desiredCode = calculateCode();

    if (conversionDelay == 0.0) {
        conversionPending = false;
        applyCode(desiredCode);
        return;
    }

    // A new input/reference value restarts the ideal conversion interval.
    if (!conversionPending && desiredCode != currentCode) {
        pendingCode = desiredCode;
        conversionReadyTime = simTime + conversionDelay;
        conversionPending = true;
    } else if (conversionPending && desiredCode != pendingCode) {
        pendingCode = desiredCode;
        conversionReadyTime = simTime + conversionDelay;
    }

    // During the conversion the previous valid output remains unchanged.
    if (conversionPending && simTime >= conversionReadyTime) {
        applyCode(pendingCode);
        conversionPending = false;
    }
}

string ADC::extraData() const {
    return to_string(bitCount) + " " + to_string(conversionDelay);
}
