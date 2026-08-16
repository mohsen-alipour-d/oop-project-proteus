#pragma once

#include "../../core/component.h"

using namespace std;

class Capacitor : public Component {
public:
    double value = 0.000001;
    double lastVoltage = 0.0;

    Capacitor() {}
    Capacitor(string name, double x, double y, double value);

    void step(double dt, double simTime) override;

    string typeTag() const override { return "C"; }
    string extraData() const override { return formatDouble(value) + " " + formatDouble(lastVoltage); }
};