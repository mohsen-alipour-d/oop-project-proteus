#pragma once

#include "../../core/component.h"

using namespace std;

class SevenSegment : public Component {
public:
    bool segOn[8] = {false};
    double logicHigh = 2.5;

    SevenSegment() {}
    SevenSegment(string name, double x, double y);

    void step(double dt, double simTime) override;
};