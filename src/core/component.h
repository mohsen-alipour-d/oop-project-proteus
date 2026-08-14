#pragma once

#include <string>
#include <vector>

#include "vector2d.h"
#include "pin.h"

using namespace std;

class Circuit;

class Component {
public:
    string name;
    Vector2D position;
    int rotation = 0;
    bool mirroredH = false;
    bool mirroredV = false;
    bool isGround = false;
    Circuit* host = nullptr;
    int id = -1;
    vector<Pin> pins;

    Component() {}
    Component(string name, double x, double y);
    virtual ~Component();

    void addPin(double x, double y);
    void rotate90();
    void mirrorHorizontal();
    void mirrorVertical();
    Vector2D pinWorldPos(const Vector2D& local) const;
    virtual void step(double dt, double simTime) {}

    virtual string typeTag() const { return "B"; }
    virtual string extraData() const { return ""; }
    string serialize() const;
};