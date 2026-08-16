#pragma once

#include "../../core/component.h"

using namespace std;

class LED : public Component {
public:
    double threshold = 2.0;
    string color = "red";
    bool lit = false;

    LED() {}
    LED(string name, double x, double y, string color);

    void step(double dt, double simTime) override;

    string typeTag() const override { return "D"; }
    string extraData() const override { return formatDouble(threshold) + " " + color; }
};