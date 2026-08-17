#include "switch_comp.h"

Switch::Switch(string name, double x, double y) : Component(name, x, y) {
    addPin(-10, 0);
    addPin(10, 0);
    pins[0].setDirection(PinDirection::Input);
    // A mechanical switch is symmetric; neither terminal is a source.
    pins[1].setDirection(PinDirection::Input);
}

void Switch::toggle() {
    closed = !closed;
}

void Switch::step(double dt, double simTime) {
    (void)dt;
    (void)simTime;
    if (closed) {
        // Bidirectional closed-switch coupling is handled by AnalogSolver.
    } else {
        pins[0].current = 0;
        pins[1].current = 0;
    }
}
