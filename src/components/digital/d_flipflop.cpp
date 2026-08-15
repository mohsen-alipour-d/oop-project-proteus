#include "d_flipflop.h"

#include <iostream>

DFlipFlop::DFlipFlop(string name, double x, double y) : Component(name, x, y) {
    addPin(-20, -10);
    addPin(-20, 10);
    addPin(20, 0);
    pins[2].isOutput = true;
}

void DFlipFlop::step(double dt, double simTime) {
    if (!pins[0].connected || !pins[1].connected) {
        if (!warned) {
            cout << "Floating input detected." << endl;
            warned = true;
        }
    }

    LogicLevel d = pins[0].connected ? voltageToLogic(pins[0].voltage) : UNDEFINED;
    LogicLevel clk = pins[1].connected ? voltageToLogic(pins[1].voltage) : UNDEFINED;

    if (clk == UNDEFINED) {
        stored = UNDEFINED;
    } else if (lastClk == LOW && clk == HIGH) {
        stored = d;
    }

    lastClk = clk;
    pins[2].voltage = logicToVoltage(stored);
}