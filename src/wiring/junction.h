#pragma once

#include <vector>

#include "../core/vector2d.h"

using namespace std;

class Wire;

class Junction {
public:
    Vector2D position;
    vector<Wire*> wires;

    Junction() {}
    Junction(double x, double y);

    void addWire(Wire* w);
    void removeWire(Wire* w);
};