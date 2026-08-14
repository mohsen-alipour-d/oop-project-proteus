#include "clock_gen.h"

#include <cmath>

ClockGenerator::ClockGenerator(string name, double x, double y, double period) : Component(name, x, y) {
    this->period = period;
    addPin(0, 0);
}

void ClockGenerator::step(double dt, double simTime) {
    double t = fmod(simTime, period);
    if (t < period / 2)
        pins[0].voltage = highVoltage;
    else
        pins[0].voltage = 0;
}