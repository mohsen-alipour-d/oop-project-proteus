#pragma once

#include "../../core/component.h"

using namespace std;

class Inductor : public Component {
public:
    double value = 0.001;
    double lastCurrent = 0.0;

    Inductor() {}
    Inductor(string name, double x, double y, double value);

    void step(double dt, double simTime) override;

    string typeTag() const override { return "L"; }
    string extraData() const override { return formatDouble(value) + " " + formatDouble(lastCurrent); }
};