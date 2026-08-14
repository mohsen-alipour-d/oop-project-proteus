#include "voltmeter.h"

#include "../core/circuit.h"

Voltmeter::Voltmeter(string name, double x, double y) : Component(name, x, y) {
    addPin(-10, 0);
    addPin(10, 0);
}

void Voltmeter::step(double dt, double simTime) {
    if (host && !host->hasGround()) {
        hasError = true;
        return;
    }
    if (!pins[0].connected || !pins[1].connected) {
        hasError = true;
        return;
    }
    hasError = false;
    reading = pins[0].voltage - pins[1].voltage;
}