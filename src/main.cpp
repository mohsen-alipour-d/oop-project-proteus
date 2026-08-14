#include <iostream>

#include "core/circuit.h"
#include "components/sources/ground.h"
#include "components/sources/dc_source.h"
#include "components/passive/resistor.h"
#include "components/digital/logic_gates.h"
#include "drc/drc.h"
#include "file/file_manager.h"
#include "file/history.h"

using namespace std;

int main() {
    Circuit c;
    FileManager fm;
    History history;

    c.addComponent(new Ground("GND1", 0, 40));
    c.addComponent(new DCSource("V1", 0, 0, 5));
    c.addComponent(new Resistor("R1", 60, 0, 100));
    c.addWire(&c.components[1]->pins[0], &c.components[2]->pins[0]);
    c.addWire(&c.components[1]->pins[1], &c.components[0]->pins[0]);

    history.push(fm.serialize(c));

    ((Resistor*)c.components[2])->value = 220;
    history.push(fm.serialize(c));

    fm.deserialize(c, history.undo());
    cout << "after undo R=" << ((Resistor*)c.components[2])->value << endl;

    fm.deserialize(c, history.redo());
    cout << "after redo R=" << ((Resistor*)c.components[2])->value << endl;

    fm.saveAs(c, "test_circuit.txt");

    Circuit loaded;
    fm.load(loaded, "test_circuit.txt");
    cout << "loaded comps=" << loaded.components.size() << " wires=" << loaded.wires.size() << endl;
    cout << "loaded R=" << ((Resistor*)loaded.components[2])->value << endl;
    cout << "loaded nets=" << loaded.nets.size() << endl;

    cout << "\n--- DRC Test 1: clean circuit ---\n";
    Circuit clean;
    clean.addComponent(new Ground("G1", 0, 0));
    clean.addComponent(new DCSource("V1", 40, 0, 5));
    clean.addComponent(new AndGate("A1", 80, 0, 2));
    clean.addWire(&clean.components[0]->pins[0], &clean.components[2]->pins[0]);
    clean.addWire(&clean.components[1]->pins[0], &clean.components[2]->pins[1]);
    clean.components[2]->pins[0].voltage = 0;
    clean.components[2]->pins[1].voltage = 5;
    DRC drc1;
    cout << "valid=" << drc1.validate(clean) << endl;
    for (LogMessage& m : drc1.log.messages)
        cout << (m.isError ? "[ERR] " : "[OK] ") << m.text << endl;

    cout << "\n--- DRC Test 2: floating input ---\n";
    Circuit floating;
    floating.addComponent(new Ground("G1", 0, 0));
    floating.addComponent(new AndGate("A1", 40, 0, 2));
    floating.addWire(&floating.components[0]->pins[0], &floating.components[1]->pins[0]);
    DRC drc2;
    cout << "valid=" << drc2.validate(floating) << endl;
    for (LogMessage& m : drc2.log.messages)
        cout << (m.isError ? "[ERR] " : "[OK] ") << m.text << endl;

    cout << "\n--- DRC Test 3: short circuit ---\n";
    Circuit shorted;
    shorted.addComponent(new Ground("G1", 0, 0));
    shorted.addComponent(new DCSource("V1", 40, 0, 5));
    shorted.addComponent(new AndGate("A1", 80, 0, 2));
    shorted.addWire(&shorted.components[0]->pins[0], &shorted.components[2]->pins[0]);
    shorted.addWire(&shorted.components[1]->pins[0], &shorted.components[2]->pins[0]);
    shorted.components[1]->pins[0].voltage = 5;
    shorted.components[0]->pins[0].voltage = 0;
    DRC drc3;
    cout << "valid=" << drc3.validate(shorted) << endl;
    for (LogMessage& m : drc3.log.messages)
        cout << (m.isError ? "[ERR] " : "[OK] ") << m.text << endl;

    return 0;
}