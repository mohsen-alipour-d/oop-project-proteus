#include "dac.h"

#include <cmath>
#include <stdexcept>

#include "../digital/logic_level.h"

DAC::DAC(string name, double x, double y, int bitCount, double conversionDelay)
    : Component(name, x, y) {
    if (bitCount < 1 || bitCount > 31)
        throw invalid_argument("DAC bit count must be between 1 and 31");

    this->bitCount = bitCount;
    setConversionDelay(conversionDelay);

    for (int bit = 0; bit < bitCount; ++bit)
        addPin(-30, bit * 10 - (bitCount - 1) * 5);

    addPin(-10, -20); // Vref+
    addPin(-10, 20);  // Vref-
    addPin(30, 0);    // Vout
    pins.back().setDirection(PinDirection::Output);
}

void DAC::setConversionDelay(double delay) {
    if (delay < 0.0)
        throw invalid_argument("DAC conversion delay cannot be negative");
    conversionDelay = delay;
}

bool DAC::readInputCode(uint32_t& code) const {
    code = 0;
    for (int bit = 0; bit < bitCount; ++bit) {
        LogicLevel level = voltageToLogic(pins[dataPinIndex(bit)].voltage);
        if (level == UNDEFINED)
            return false;
        if (level == HIGH)
            code |= (uint32_t(1) << bit);
    }
    return true;
}

double DAC::calculateVoltage(uint32_t code) const {
    const double vrefPlus = pins[vrefPlusPinIndex()].voltage;
    const double vrefMinus = pins[vrefMinusPinIndex()].voltage;
    const uint32_t maxCode = (uint32_t(1) << bitCount) - 1u;
    if (maxCode == 0)
        return vrefMinus;
    const double ratio = static_cast<double>(code) / static_cast<double>(maxCode);
    return vrefMinus + ratio * (vrefPlus - vrefMinus);
}

void DAC::applyVoltage(double voltage) {
    currentVoltage = voltage;
    pins[outputPinIndex()].voltage = voltage;
}

void DAC::step(double dt, double simTime) {
    (void)dt;
    uint32_t code = 0;
    if (!readInputCode(code))
        return;

    const double desiredVoltage = calculateVoltage(code);

    if (conversionDelay == 0.0) {
        conversionPending = false;
        lastInputCode = code;
        applyVoltage(desiredVoltage);
        return;
    }

    if (!conversionPending && (code != lastInputCode || fabs(desiredVoltage - currentVoltage) > 1e-12)) {
        pendingInputCode = code;
        pendingVoltage = desiredVoltage;
        conversionReadyTime = simTime + conversionDelay;
        conversionPending = true;
    } else if (conversionPending &&
               (code != pendingInputCode || fabs(desiredVoltage - pendingVoltage) > 1e-12)) {
        pendingInputCode = code;
        pendingVoltage = desiredVoltage;
        conversionReadyTime = simTime + conversionDelay;
    }

    if (conversionPending && simTime >= conversionReadyTime) {
        lastInputCode = pendingInputCode;
        applyVoltage(pendingVoltage);
        conversionPending = false;
    }
}

string DAC::extraData() const {
    return to_string(bitCount) + " " + to_string(conversionDelay);
}
