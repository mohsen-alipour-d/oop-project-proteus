#pragma once

#include "../core/component.h"

using namespace std;

class Ammeter : public Component {
public:
    double reading = 0.0;

    Ammeter() {}
    Ammeter(string name, double x, double y);

    void step(double dt, double simTime) override;
};