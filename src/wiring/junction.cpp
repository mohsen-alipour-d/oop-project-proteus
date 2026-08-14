#include "junction.h"

#include "wire.h"

Junction::Junction(double x, double y) {
    position = Vector2D(x, y);
}

void Junction::addWire(Wire* w) {
    wires.push_back(w);
}

void Junction::removeWire(Wire* w) {
    for (int i = 0; i < (int)wires.size(); i++) {
        if (wires[i] == w) {
            wires.erase(wires.begin() + i);
            return;
        }
    }
}