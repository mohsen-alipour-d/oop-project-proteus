#include <iostream>

#include "core/component.h"
#include "components/passive/resistor.h"
#include "components/passive/capacitor.h"
#include "components/passive/inductor.h"

using namespace std;

int main() {
    Resistor r("R1", 0, 0, 100);
    r.pins[0].voltage = 5;
    r.pins[1].voltage = 0;
    r.step(0.01, 0);
    cout << "resistor current " << r.pins[0].current << endl;

    Capacitor c("C1", 40, 0, 0.000001);
    c.pins[0].voltage = 5;
    c.pins[1].voltage = 0;
    c.step(0.01, 0);
    cout << "capacitor current " << c.pins[0].current << endl;

    Inductor l("L1", 80, 0, 0.001);
    l.pins[0].voltage = 5;
    l.pins[1].voltage = 0;
    for (int i = 0; i < 3; i++)
        l.step(0.01, i * 0.01);
    cout << "inductor current " << l.pins[0].current << endl;

    return 0;
}