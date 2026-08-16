#include "switch_comp.h"

Switch::Switch(string name, double x, double y) : Component(name, x, y) {
    addPin(-10, 0);
    addPin(10, 0);
    pins[0].setDirection(PinDirection::Input);
    pins[1].setDirection(PinDirection::Bidirectional);
}

void Switch::toggle() {
    closed = !closed;
}

void Switch::step(double dt, double simTime) {
    if (closed) {
        pins[1].setDriving(true);
        pins[1].voltage = pins[0].voltage;
        pins[0].current = -pins[1].current;
    } else {
        pins[1].setDriving(false);
        pins[0].current = 0;
        pins[1].current = 0;
    }
}
