#pragma once

#include "../../core/component.h"

using namespace std;

class Resistor : public Component {
public:
    double value = 1000.0;

    Resistor() {}
    Resistor(string name, double x, double y, double value);

    void step(double dt, double simTime) override;
};