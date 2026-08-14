#include <iostream>

#include "core/circuit.h"
#include "components/sources/ground.h"
#include "components/sources/dc_source.h"
#include "components/passive/resistor.h"
#include "file/file_manager.h"
#include "file/history.h"
#include "measurement/voltmeter.h"
#include "measurement/ammeter.h"

using namespace std;

int main() {
    Circuit circuit;
    FileManager fm;
    History history;

    circuit.addComponent(new Ground("GND1", 0, 40));
    circuit.addComponent(new DCSource("V1", 0, 0, 5));
    circuit.addComponent(new Resistor("R1", 60, 0, 100));
    circuit.addWire(&circuit.components[1]->pins[0], &circuit.components[2]->pins[0]);
    circuit.addWire(&circuit.components[1]->pins[1], &circuit.components[0]->pins[0]);

    history.push(fm.serialize(circuit));

    ((Resistor*)circuit.components[2])->value = 220;
    history.push(fm.serialize(circuit));

    fm.deserialize(circuit, history.undo());
    cout << "after undo R=" << ((Resistor*)circuit.components[2])->value << endl;

    fm.deserialize(circuit, history.redo());
    cout << "after redo R=" << ((Resistor*)circuit.components[2])->value << endl;

    fm.saveAs(circuit, "test_circuit.txt");

    Circuit loaded;
    fm.load(loaded, "test_circuit.txt");
    cout << "loaded comps=" << loaded.components.size() << " wires=" << loaded.wires.size() << endl;
    cout << "loaded R=" << ((Resistor*)loaded.components[2])->value << endl;
    cout << "loaded nets=" << loaded.nets.size() << endl;

    return 0;
}