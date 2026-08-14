#include "battery.h"

Battery::Battery(string name, double x, double y, double value) : Component(name, x, y) {
    this->value = value;
    addPin(0, -10);
    addPin(0, 10);
    pins[0].isOutput = true;
    pins[1].isOutput = true;
}

void Battery::step(double dt, double simTime) {
    double i = pins[0].current;
    pins[0].voltage = pins[1].voltage + value - i * internalResistance;
}