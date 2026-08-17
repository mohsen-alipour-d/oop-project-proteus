#include "battery.h"

Battery::Battery(string name, double x, double y, double value) : Component(name, x, y) {
    this->value = value;
    addPin(0, -10);
    addPin(0, 10);
    // Match the frontend definition: pin 0 is '-', pin 1 is '+'.  A battery
    // is a relative two-terminal source, not two independent output drivers.
    pins[0].setDirection(PinDirection::Input);
    pins[1].setDirection(PinDirection::Input);
    pins[0].voltage = 0.0;
    pins[1].voltage = value;
}

void Battery::step(double dt, double simTime) {
    (void)dt;
    (void)simTime;
    // Standalone fallback; inside a Circuit the nodal solver owns this
    // voltage constraint and also accounts for internalResistance.
    pins[1].voltage = pins[0].voltage + value -
                      pins[1].current * internalResistance;
}
