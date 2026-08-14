#pragma once

#include <string>
#include <vector>

#include "vector2d.h"
#include "pin.h"

using namespace std;

class Component {
public:
    string name;
    Vector2D position;
    int rotation = 0;
    bool mirroredH = false;
    bool mirroredV = false;
    vector<Pin> pins;

    Component() {}
    Component(string name, double x, double y);
    virtual ~Component();

    void addPin(double x, double y);
    void rotate90();
    void mirrorHorizontal();
    void mirrorVertical();
    Vector2D pinWorldPos(const Vector2D& local) const;
};