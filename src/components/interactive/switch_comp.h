#pragma once

#include "../../core/component.h"

using namespace std;

class Switch : public Component {
public:
    bool closed = false;

    Switch() {}
    Switch(string name, double x, double y);

    void toggle();
    void step(double dt, double simTime) override;
};