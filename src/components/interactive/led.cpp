#include "led.h"

LED::LED(string name, double x, double y, string color) : Component(name, x, y) {
    this->color = color;
    addPin(-10, 0);
    addPin(10, 0);
}

void LED::step(double dt, double simTime) {
    double v = pins[0].voltage - pins[1].voltage;
    if (v > threshold && v < threshold + 0.5) {
        lit = true;
        pins[0].current = (v - threshold) / 100.0;
    } else if (v >= threshold + 0.5) {
        lit = true;
        pins[0].current = 0.02;
    } else {
        lit = false;
        pins[0].current = 0;
    }
    pins[1].current = -pins[0].current;
}