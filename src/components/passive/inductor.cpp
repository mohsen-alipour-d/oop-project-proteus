#include "inductor.h"

Inductor::Inductor(string name, double x, double y, double value) : Component(name, x, y) {
    this->value = value;
    addPin(-10, 0);
    addPin(10, 0);
}

void Inductor::step(double dt, double simTime) {
    if (dt <= 0.0)
        return;
    double v = pins[0].voltage - pins[1].voltage;
    lastCurrent = lastCurrent + (v / value) * dt;
    pins[0].current = lastCurrent;
    pins[1].current = -lastCurrent;
}