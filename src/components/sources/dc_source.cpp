#include "dc_source.h"

DCSource::DCSource(string name, double x, double y, double value) : Component(name, x, y) {
    this->value = value;
    addPin(0, -10);
    addPin(0, 10);
    // Match the frontend definition: pin 0 is '-', pin 1 is '+'.
    pins[0].setDirection(PinDirection::Input);
    pins[1].setDirection(PinDirection::Input);
    pins[0].voltage = 0.0;
    pins[1].voltage = value;
}

void DCSource::step(double dt, double simTime) {
    (void)dt;
    (void)simTime;
    pins[1].voltage = pins[0].voltage + value;
}
