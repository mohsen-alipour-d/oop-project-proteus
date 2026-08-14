#pragma once

#include <cmath>

using namespace std;

struct Vector2D {
    double x = 0;
    double y = 0;

    Vector2D() {}

    Vector2D(double x, double y) {
        this->x = x;
        this->y = y;
    }

    Vector2D operator+(const Vector2D& o) const {
        return Vector2D(x + o.x, y + o.y);
    }

    Vector2D operator-(const Vector2D& o) const {
        return Vector2D(x - o.x, y - o.y);
    }

    double distanceTo(const Vector2D& o) const {
        double dx = x - o.x;
        double dy = y - o.y;
        return sqrt(dx * dx + dy * dy);
    }
};