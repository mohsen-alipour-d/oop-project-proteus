#include "dc_source.h"

DCSource::DCSource(string name, double x, double y, double value) : Component(name, x, y) {
    this->value = value;
    addPin(0, -10);
    addPin(0, 10);
}

void DCSource::step(double dt, double simTime) {
    pins[0].voltage = pins[1].voltage + value;
}