#include <iostream>

#include "core/component.h"
#include "core/circuit.h"
#include "components/sources/dc_source.h"
#include "components/passive/resistor.h"

using namespace std;

int main() {
    Circuit circuit;

    DCSource src("V1", 0, 0, 5);
    Resistor res("R1", 60, 0, 100);
    circuit.addComponent(&src);
    circuit.addComponent(&res);

    Wire* w1 = circuit.addWire(&src.pins[0], &res.pins[0]);
    cout << "wire points " << w1->points.size() << endl;
    for (Vector2D p : w1->points)
        cout << p.x << "," << p.y << " ";
    cout << endl;

    cout << "nets " << circuit.nets.size() << endl;
    cout << "src pin0 connected " << src.pins[0].connected << endl;

    res.position = Vector2D(80, 20);
    circuit.refreshWires();
    for (Vector2D p : w1->points)
        cout << p.x << "," << p.y << " ";
    cout << endl;

    circuit.removeWire(w1);
    cout << "after delete nets " << circuit.nets.size() << endl;
    cout << "src pin0 connected " << src.pins[0].connected << endl;

    return 0;
}