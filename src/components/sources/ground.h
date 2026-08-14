#pragma once

#include "../../core/component.h"

using namespace std;

class Ground : public Component {
public:
    Ground() {}
    Ground(string name, double x, double y);

    void step(double dt, double simTime) override;
};