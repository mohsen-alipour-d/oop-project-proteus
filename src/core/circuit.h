#pragma once

#include <vector>

#include "component.h"
#include "../wiring/wire.h"
#include "../wiring/junction.h"
#include "../wiring/net.h"

using namespace std;

class Circuit {
public:
    vector<Component*> components;
    vector<Wire*> wires;
    vector<Junction*> junctions;
    vector<Net*> nets;
    int nextComponentId = 0;
    int nextWireId = 0;

    Circuit() {}
    ~Circuit();

    void clear();
    void addComponent(Component* c);
    void removeComponent(Component* c);
    Wire* addWire(Pin* a, Pin* b);
    void removeWire(Wire* w);
    void removeNetOf(Wire* w);
    Junction* addJunctionAt(const Vector2D& pos);
    void refreshWires();
    void rebuildNets();
    bool hasGround();
    void propagateVoltages();
    void moveComponent(Component* c, double dx, double dy);
    vector<Component*> getComponentsInRect(const Rect& r);
    Pin* findPinAt(const Vector2D& pos, double tol);
    Wire* findWireAt(const Vector2D& pos, double tol);
};