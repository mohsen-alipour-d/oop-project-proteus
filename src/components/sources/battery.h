#pragma once

#include "../../core/component.h"

using namespace std;

class Battery : public Component {
public:
    double value = 9.0;
    double internalResistance = 0.0;

    Battery() {}
    Battery(string name, double x, double y, double value);

    void step(double dt, double simTime) override;

    string typeTag() const override { return "B"; }
    string extraData() const override { return to_string(value) + " " + to_string(internalResistance); }
};