#include "ammeter.h"

Ammeter::Ammeter(string name, double x, double y) : Component(name, x, y) {
    addPin(-10, 0);
    addPin(10, 0);
}

void Ammeter::step(double dt, double simTime) {
    if (pins[0].connected)
        reading = pins[0].current;
    else
        reading = 0.0;
}