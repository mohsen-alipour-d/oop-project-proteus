#include "led.h"

LED::LED(string name, double x, double y, string color) : Component(name, x, y) {
    this->color = color;
    addPin(-10, 0);
    addPin(10, 0);
}

void LED::step(double dt, double simTime) {
    double v = pins[0].voltage - pins[1].voltage;
    lit = v > threshold;
    if (lit)
        pins[0].current = (v - threshold) / 100.0;
    else
        pins[0].current = 0;
    pins[1].current = -pins[0].current;
}