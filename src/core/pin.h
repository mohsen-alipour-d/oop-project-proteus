#pragma once

#include "vector2d.h"

using namespace std;

class Component;

class Pin {
public:
    Vector2D localPos;
    double sensitivityRadius = 5.0;
    bool isHighlighted = false;
    double voltage = 0.0;
    double current = 0.0;
    bool connected = false;
    Component* owner = nullptr;

    Pin() {}
    Pin(double x, double y, Component* owner);

    Vector2D worldPos() const;
    bool checkMouseOver(const Vector2D& mousePos);
};