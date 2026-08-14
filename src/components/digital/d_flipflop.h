#pragma once

#include "../../core/component.h"
#include "logic_level.h"

using namespace std;

class DFlipFlop : public Component {
public:
    DFlipFlop() {}
    DFlipFlop(string name, double x, double y);

    void step(double dt, double simTime) override;

    string typeTag() const override { return "F"; }

private:
    LogicLevel stored = LOW;
    LogicLevel lastClk = LOW;
    bool warned = false;
};