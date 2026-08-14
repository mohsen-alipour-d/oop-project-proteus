#pragma once

#include <vector>

#include "../core/vector2d.h"
#include "../core/pin.h"

using namespace std;

class Wire {
public:
    vector<Vector2D> points;
    Pin* startPin = nullptr;
    Pin* endPin = nullptr;
    bool isSelected = false;
    int netId = -1;

    Wire() {}
    Wire(Pin* start, Pin* end);

    void route90();
    double distanceTo(const Vector2D& p) const;
    bool containsPoint(const Vector2D& p, double tol) const;
};