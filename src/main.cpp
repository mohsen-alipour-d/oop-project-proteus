#include <iostream>

#include "core/component.h"
#include "components/digital/logic_gates.h"
#include "components/digital/d_flipflop.h"

using namespace std;

int main() {
    AndGate and1("AND1", 0, 0, 2);
    and1.pins[0].connected = true;
    and1.pins[1].connected = true;
    and1.pins[0].voltage = 5;
    and1.pins[1].voltage = 5;
    and1.step(0.000001, 0);
    and1.step(0.000001, 0.000002);
    cout << "and high " << and1.pins[2].voltage << endl;

    and1.pins[1].voltage = 0;
    and1.step(0.000001, 0.000003);
    and1.step(0.000001, 0.000005);
    cout << "and low " << and1.pins[2].voltage << endl;

    OrGate or1("OR1", 60, 0, 2);
    or1.pins[0].connected = true;
    or1.pins[0].voltage = 0;
    or1.step(0.000001, 0);
    or1.step(0.000001, 0.000002);
    cout << "or floating " << or1.pins[2].voltage << endl;

    NotGate not1("NOT1", 120, 0);
    not1.pins[0].connected = true;
    not1.pins[0].voltage = 5;
    not1.step(0.000001, 0);
    not1.step(0.000001, 0.000002);
    cout << "not out " << not1.pins[1].voltage << endl;

    XorGate xor1("XOR1", 180, 0, 2);
    xor1.pins[0].connected = true;
    xor1.pins[1].connected = true;
    xor1.pins[0].voltage = 5;
    xor1.pins[1].voltage = 0;
    xor1.step(0.000001, 0);
    xor1.step(0.000001, 0.000002);
    cout << "xor out " << xor1.pins[2].voltage << endl;

    NandGate nand1("NAND1", 240, 0, 2);
    nand1.pins[0].connected = true;
    nand1.pins[1].connected = true;
    nand1.pins[0].voltage = 5;
    nand1.pins[1].voltage = 5;
    nand1.step(0.000001, 0);
    nand1.step(0.000001, 0.000002);
    cout << "nand out " << nand1.pins[2].voltage << endl;

    DFlipFlop ff("FF1", 300, 0);
    ff.pins[0].connected = true;
    ff.pins[1].connected = true;
    ff.pins[0].voltage = 5;
    ff.pins[1].voltage = 0;
    ff.step(0.000001, 0);
    ff.pins[1].voltage = 5;
    ff.step(0.000001, 0.000001);
    cout << "ff q after edge " << ff.pins[2].voltage << endl;
    ff.pins[0].voltage = 0;
    ff.step(0.000001, 0.000002);
    cout << "ff q no edge " << ff.pins[2].voltage << endl;
    ff.pins[1].voltage = 0;
    ff.step(0.000001, 0.000003);
    ff.pins[1].voltage = 5;
    ff.step(0.000001, 0.000004);
    cout << "ff q second edge " << ff.pins[2].voltage << endl;

    return 0;
}