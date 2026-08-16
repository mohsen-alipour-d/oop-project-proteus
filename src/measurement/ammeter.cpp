#include "ammeter.h"

#include "../core/circuit.h"
#include "../wiring/net.h"

Ammeter::Ammeter(string name, double x, double y) : Component(name, x, y) {
    addPin(-10, 0);
    addPin(10, 0);
}

void Ammeter::step(double dt, double simTime) {
    reading = 0;
    if (!pins[0].connected || !pins[1].connected)
        return;
    if (!host)
        return;

    for (Net* n : host->nets) {
        if (!n->hasPin(&pins[0]) && !n->hasPin(&pins[1]))
            continue;
        for (Pin* p : n->pins) {
            if (p->owner == this)
                continue;
            if (p->current != 0) {
                reading = p->current;
                return;
            }
        }
    }
}