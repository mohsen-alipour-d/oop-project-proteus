#include "logic_gate.h"

#include <iostream>

LogicGate::LogicGate(string name, double x, double y, int inputCount) : Component(name, x, y) {
    for (int i = 0; i < inputCount; i++)
        addPin(-20, i * 10 - (inputCount - 1) * 5);
    addPin(20, 0);
}

void LogicGate::step(double dt, double simTime) {
    vector<LogicLevel> ins;
    bool hasUndefined = false;

    for (int i = 0; i + 1 < (int)pins.size(); i++) {
        LogicLevel l;
        if (!pins[i].connected) {
            l = UNDEFINED;
            if (!warned) {
                cout << "Floating input detected on " << name << endl;
                warned = true;
            }
        } else {
            l = voltageToLogic(pins[i].voltage);
        }
        if (l == UNDEFINED)
            hasUndefined = true;
        ins.push_back(l);
    }

    LogicLevel out = hasUndefined ? UNDEFINED : apply(ins);

    if (out != lastOut) {
        lastOut = out;
        changeTime = simTime;
    }
    if (simTime - changeTime >= propagationDelay)
        pins.back().voltage = logicToVoltage(out);
}