#include "voltage_probe.h"

double VoltageProbe::read(Circuit& circuit, const Vector2D& pos) {
    Pin* p = circuit.findPinAt(pos, 5.0);
    if (p)
        return p->voltage;

    Wire* w = circuit.findWireAt(pos, 3.0);
    if (w && w->netId >= 0 && w->netId < (int)circuit.nets.size())
        return circuit.nets[w->netId]->voltage;

    return 0.0;
}

bool VoltageProbe::isFloating(Circuit& circuit, const Vector2D& pos) {
    Pin* p = circuit.findPinAt(pos, 5.0);
    if (p) {
        if (!p->connected)
            return true;
        for (Net* net : circuit.nets)
            if (net->hasPin(p))
                return !net->voltageResolved;
        return true;
    }

    Wire* w = circuit.findWireAt(pos, 3.0);
    if (w && w->netId >= 0 && w->netId < (int)circuit.nets.size())
        return !circuit.nets[w->netId]->voltageResolved;

    return true;
}
