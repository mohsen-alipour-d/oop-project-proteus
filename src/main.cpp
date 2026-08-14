#include <iostream>

#include "core/component.h"
#include "core/circuit.h"
#include "components/sources/dc_source.h"
#include "components/sources/ground.h"
#include "components/passive/resistor.h"
#include "measurement/voltage_probe.h"
#include "measurement/voltmeter.h"
#include "measurement/ammeter.h"
#include "measurement/oscilloscope.h"

using namespace std;

int main() {
    Circuit circuit;
    Ground g("GND1", 0, 40);
    DCSource src("V1", 0, 0, 5);
    Resistor res("R1", 60, 0, 100);
    circuit.addComponent(&g);
    circuit.addComponent(&src);
    circuit.addComponent(&res);

    Wire* w1 = circuit.addWire(&src.pins[0], &res.pins[0]);
    Wire* w2 = circuit.addWire(&src.pins[1], &res.pins[1]);

    src.pins[1].voltage = 0;
    src.step(0.01, 0);
    res.step(0.01, 0);

    for (Net* n : circuit.nets) {
        if (n->hasPin(&src.pins[0]))
            n->voltage = 5;
        if (n->hasPin(&src.pins[1]))
            n->voltage = 0;
    }

    VoltageProbe probe;
    cout << "probe at src+ " << probe.read(circuit, src.pins[0].worldPos()) << endl;
    cout << "probe floating " << probe.isFloating(circuit, Vector2D(1000, 1000)) << endl;

    Voltmeter vm("VM1", 0, 80);
    circuit.addComponent(&vm);
    vm.pins[0].voltage = 5;
    vm.pins[1].voltage = 0;
    vm.pins[0].connected = true;
    vm.pins[1].connected = true;
    vm.step(0.01, 0);
    cout << "voltmeter " << vm.reading << " err " << vm.hasError << endl;

    Ammeter am("AM1", 0, 120);
    circuit.addComponent(&am);
    am.pins[0].current = 0.05;
    am.pins[0].connected = true;
    am.step(0.01, 0);
    cout << "ammeter " << am.reading << endl;

    Oscilloscope scope;
    scope.isRunning = true;
    scope.attachChannel(0, 0);
    for (int i = 0; i < 3; i++)
        scope.update(circuit, i * 0.1);
    cout << "scope ch0 points " << scope.channels[0].history.size() << endl;

    scope.pause();
    scope.update(circuit, 0.4);
    cout << "scope paused points " << scope.channels[0].history.size() << endl;

    scope.stop();
    cout << "scope stopped points " << scope.channels[0].history.size() << endl;

    return 0;
}