#include "ground.h"

Ground::Ground(string name, double x, double y) : Component(name, x, y) {
    addPin(0, 0);
}

void Ground::step(double dt, double simTime) {
    pins[0].voltage = 0;
}