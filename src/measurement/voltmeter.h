#pragma once

#include "../core/component.h"

using namespace std;

class Voltmeter : public Component {
public:
    double reading = 0.0;
    bool hasError = false;

    Voltmeter() {}
    Voltmeter(string name, double x, double y);

    void step(double dt, double simTime) override;

    string typeTag() const override { return "M"; }
};