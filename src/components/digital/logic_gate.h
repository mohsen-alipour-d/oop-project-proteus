#pragma once

#include <vector>

#include "../../core/component.h"
#include "logic_level.h"

using namespace std;

class LogicGate : public Component {
public:
    double propagationDelay = 0.000001;

    LogicGate() {}
    LogicGate(string name, double x, double y, int inputCount);

    virtual LogicLevel apply(vector<LogicLevel>& ins) = 0;
    void step(double dt, double simTime) override;

private:
    LogicLevel lastOut = LOW;
    double changeTime = 0;
    bool warned = false;
};