#pragma once

#include <cstdint>
#include <string>

#include "../../core/component.h"

using namespace std;

class ADC : public Component {
public:
    static constexpr int VIN_PIN = 0;
    static constexpr int VREF_PLUS_PIN = 1;
    static constexpr int VREF_MINUS_PIN = 2;

    ADC() {}
    ADC(string name, double x, double y, int bitCount, double conversionDelay);

    void step(double dt, double simTime) override;

    int getBitCount() const { return bitCount; }
    double getConversionDelay() const { return conversionDelay; }
    void setConversionDelay(double delay);
    uint32_t getOutputCode() const { return currentCode; }
    int outputPinIndex(int bit) const { return 3 + bit; }

    string typeTag() const override { return "ADC"; }
    string extraData() const override;

private:
    int bitCount = 8;
    double conversionDelay = 0.0;
    uint32_t currentCode = 0;
    uint32_t pendingCode = 0;
    double conversionReadyTime = 0.0;
    bool conversionPending = false;

    uint32_t calculateCode() const;
    void applyCode(uint32_t code);
};
