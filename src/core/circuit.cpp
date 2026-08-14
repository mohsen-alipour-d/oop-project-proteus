#include "circuit.h"

static int ufFind(vector<int>& parent, int i) {
    while (parent[i] != i) {
        parent[i] = parent[parent[i]];
        i = parent[i];
    }
    return i;
}

static void ufUnion(vector<int>& parent, int a, int b) {
    int ra = ufFind(parent, a);
    int rb = ufFind(parent, b);
    if (ra != rb)
        parent[rb] = ra;
}

Circuit::~Circuit() {
    for (Wire* w : wires)
        delete w;
    for (Junction* j : junctions)
        delete j;
    for (Net* n : nets)
        delete n;
}

void Circuit::addComponent(Component* c) {
    components.push_back(c);
}

void Circuit::removeComponent(Component* c) {
    vector<Wire*> toDelete;
    for (Wire* w : wires) {
        for (Pin& p : c->pins) {
            if (w->startPin == &p || w->endPin == &p) {
                toDelete.push_back(w);
                break;
            }
        }
    }
    for (Wire* w : toDelete) {
        for (int i = 0; i < (int)wires.size(); i++) {
            if (wires[i] == w) {
                wires.erase(wires.begin() + i);
                break;
            }
        }
        for (Junction* j : junctions)
            j->removeWire(w);
        delete w;
    }
    for (int i = 0; i < (int)components.size(); i++) {
        if (components[i] == c) {
            components.erase(components.begin() + i);
            break;
        }
    }
    rebuildNets();
}

Wire* Circuit::addWire(Pin* a, Pin* b) {
    Wire* w = new Wire(a, b);
    wires.push_back(w);
    rebuildNets();
    return w;
}

void Circuit::removeWire(Wire* w) {
    for (int i = 0; i < (int)wires.size(); i++) {
        if (wires[i] == w) {
            wires.erase(wires.begin() + i);
            break;
        }
    }
    for (Junction* j : junctions)
        j->removeWire(w);
    for (int i = 0; i < (int)junctions.size(); i++) {
        if (junctions[i]->wires.empty()) {
            delete junctions[i];
            junctions.erase(junctions.begin() + i);
            i--;
        }
    }
    delete w;
    rebuildNets();
}

void Circuit::removeNetOf(Wire* w) {
    int target = w->netId;
    if (target < 0) {
        removeWire(w);
        return;
    }
    vector<Wire*> toDelete;
    for (Wire* x : wires) {
        if (x->netId == target)
            toDelete.push_back(x);
    }
    for (Wire* x : toDelete) {
        for (int i = 0; i < (int)wires.size(); i++) {
            if (wires[i] == x) {
                wires.erase(wires.begin() + i);
                break;
            }
        }
        for (Junction* j : junctions)
            j->removeWire(x);
        delete x;
    }
    for (int i = 0; i < (int)junctions.size(); i++) {
        if (junctions[i]->wires.empty()) {
            delete junctions[i];
            junctions.erase(junctions.begin() + i);
            i--;
        }
    }
    rebuildNets();
}

Junction* Circuit::addJunctionAt(const Vector2D& pos) {
    Junction* j = new Junction(pos.x, pos.y);
    for (Wire* w : wires) {
        if (w->containsPoint(pos, 2.0))
            j->addWire(w);
    }
    if ((int)j->wires.size() < 2) {
        delete j;
        return nullptr;
    }
    junctions.push_back(j);
    rebuildNets();
    return j;
}

void Circuit::refreshWires() {
    for (Wire* w : wires)
        w->route90();
}

void Circuit::rebuildNets() {
    for (Net* n : nets)
        delete n;
    nets.clear();

    vector<Pin*> allPins;
    for (Component* c : components) {
        for (Pin& p : c->pins)
            allPins.push_back(&p);
    }

    int n = (int)allPins.size();
    vector<int> parent(n);
    for (int i = 0; i < n; i++) {
        parent[i] = i;
        allPins[i]->connected = false;
    }

    auto indexOf = [&](Pin* p) {
        for (int i = 0; i < n; i++) {
            if (allPins[i] == p)
                return i;
        }
        return -1;
    };

    for (Wire* w : wires) {
        int a = indexOf(w->startPin);
        int b = indexOf(w->endPin);
        if (a >= 0 && b >= 0)
            ufUnion(parent, a, b);
    }

    for (Junction* j : junctions) {
        if ((int)j->wires.size() < 2)
            continue;
        int first = indexOf(j->wires[0]->startPin);
        if (first < 0)
            continue;
        for (int i = 0; i < (int)j->wires.size(); i++) {
            int a = indexOf(j->wires[i]->startPin);
            int b = indexOf(j->wires[i]->endPin);
            if (a >= 0)
                ufUnion(parent, first, a);
            if (b >= 0)
                ufUnion(parent, first, b);
        }
    }

    for (int i = 0; i < n; i++) {
        if (ufFind(parent, i) != i)
            continue;
        int cnt = 0;
        for (int k = 0; k < n; k++) {
            if (ufFind(parent, k) == i)
                cnt++;
        }
        if (cnt < 2)
            continue;
        Net* net = new Net((int)nets.size());
        for (int k = 0; k < n; k++) {
            if (ufFind(parent, k) == i) {
                net->addPin(allPins[k]);
                allPins[k]->connected = true;
            }
        }
        nets.push_back(net);
    }

    for (Wire* w : wires) {
        w->netId = -1;
        for (Net* net : nets) {
            if (net->hasPin(w->startPin))
                w->netId = net->id;
        }
    }
}

Pin* Circuit::findPinAt(const Vector2D& pos, double tol) {
    for (Component* c : components) {
        for (Pin& p : c->pins) {
            if (p.worldPos().distanceTo(pos) <= tol)
                return &p;
        }
    }
    return nullptr;
}

Wire* Circuit::findWireAt(const Vector2D& pos, double tol) {
    for (Wire* w : wires) {
        if (w->containsPoint(pos, tol))
            return w;
    }
    return nullptr;
}