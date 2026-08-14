#pragma once

#include "../../core/component.h"

using namespace std;

class ClockGenerator : public Component {
public:
    double period = 1.0;
    double highVoltage = 5.0;

    ClockGenerator() {}
    ClockGenerator(string name, double x, double y, double period);

    void step(double dt, double simTime) override;
};