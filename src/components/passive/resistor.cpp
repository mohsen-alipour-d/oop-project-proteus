#include "resistor.h"

Resistor::Resistor(string name, double x, double y, double value) : Component(name, x, y) {
    this->value = value;
    addPin(-10, 0);
    addPin(10, 0);
}

void Resistor::step(double dt, double simTime) {
    double r = value > 0.0 ? value : 1e-9;
    double i = (pins[0].voltage - pins[1].voltage) / r;    pins[0].current = i;
    pins[1].current = -i;
}