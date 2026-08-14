#pragma once

#include "../../core/component.h"

using namespace std;

class DCSource : public Component {
public:
    double value = 5.0;

    DCSource() {}
    DCSource(string name, double x, double y, double value);

    void step(double dt, double simTime) override;
};