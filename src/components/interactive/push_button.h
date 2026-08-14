#pragma once

#include "../../core/component.h"

using namespace std;

class PushButton : public Component {
public:
    bool pressed = false;
    double highVoltage = 5.0;

    PushButton() {}
    PushButton(string name, double x, double y);

    void press();
    void release();
    void step(double dt, double simTime) override;
};