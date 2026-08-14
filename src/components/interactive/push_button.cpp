#include "push_button.h"

PushButton::PushButton(string name, double x, double y) : Component(name, x, y) {
    addPin(0, 0);
    pins[0].isOutput = true;
}

void PushButton::press() {
    pressed = true;
}

void PushButton::release() {
    pressed = false;
}

void PushButton::step(double dt, double simTime) {
    pins[0].voltage = pressed ? highVoltage : 0;
}