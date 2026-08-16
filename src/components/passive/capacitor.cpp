#include "capacitor.h"

Capacitor::Capacitor(string name, double x, double y, double value) : Component(name, x, y) {
    this->value = value;
    addPin(-10, 0);
    addPin(10, 0);
}

void Capacitor::step(double dt, double simTime) {
    if (dt <= 0.0)
        return;
    double v = pins[0].voltage - pins[1].voltage;
    double i = value * (v - lastVoltage) / dt;
    lastVoltage = v;
    pins[0].current = i;
    pins[1].current = -i;
}