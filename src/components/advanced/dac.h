#pragma once

#include <cstdint>
#include <string>

#include "../../core/component.h"

using namespace std;

class DAC : public Component {
public:
    DAC() {}
    DAC(string name, double x, double y, int bitCount, double conversionDelay);

    void step(double dt, double simTime) override;

    int getBitCount() const { return bitCount; }
    double getConversionDelay() const { return conversionDelay; }
    void setConversionDelay(double delay);
    double getOutputVoltage() const { return currentVoltage; }

    int dataPinIndex(int bit) const { return bit; }
    int vrefPlusPinIndex() const { return bitCount; }
    int vrefMinusPinIndex() const { return bitCount + 1; }
    int outputPinIndex() const { return bitCount + 2; }

    string typeTag() const override { return "DAC"; }
    string extraData() const override;

private:
    int bitCount = 8;
    double conversionDelay = 0.0;
    double currentVoltage = 0.0;
    double pendingVoltage = 0.0;
    uint32_t lastInputCode = 0;
    uint32_t pendingInputCode = 0;
    double conversionReadyTime = 0.0;
    bool conversionPending = false;

    bool readInputCode(uint32_t& code) const;
    double calculateVoltage(uint32_t code) const;
    void applyVoltage(double voltage);
};
