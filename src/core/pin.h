#pragma once

#include "vector2d.h"

using namespace std;

class Component;

enum class PinDirection {
    Input,
    Output,
    Bidirectional
};

class Pin {
public:
    Vector2D localPos;
    double sensitivityRadius = 5.0;
    bool isHighlighted = false;
    double voltage = 0.0;
    double current = 0.0;
    bool connected = false;

    // Kept for backward compatibility with the already implemented parts.
    // New code should prefer direction/setDirection()/drivesNet().
    bool isOutput = false;
    PinDirection direction = PinDirection::Input;
    bool driving = false;

    int id = -1;
    Component* owner = nullptr;

    Pin() {}
    Pin(double x, double y, Component* owner);

    Vector2D worldPos() const;
    bool checkMouseOver(const Vector2D& mousePos);

    void setDirection(PinDirection newDirection);
    void setDriving(bool enabled);
    bool drivesNet() const;
    bool needsInputConnection() const;
};
