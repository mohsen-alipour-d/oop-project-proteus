#pragma once

#include "../../core/component.h"
#include "logic_level.h"

using namespace std;

class DFlipFlop : public Component {
public:
    LogicLevel stored = LOW;
    LogicLevel lastClk = LOW;

    DFlipFlop() {}
    DFlipFlop(string name, double x, double y);

    void step(double dt, double simTime) override;

    string typeTag() const override { return "F"; }
    string extraData() const override { return to_string((int)stored) + " " + to_string((int)lastClk); }

private:
    bool warned = false;
};