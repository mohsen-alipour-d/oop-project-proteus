#include "component_factory.h"

#include "../components/sources/ground.h"
#include "../components/sources/dc_source.h"
#include "../components/sources/battery.h"
#include "../components/sources/clock_gen.h"
#include "../components/passive/resistor.h"
#include "../components/passive/capacitor.h"
#include "../components/passive/inductor.h"
#include "../components/interactive/switch_comp.h"
#include "../components/interactive/push_button.h"
#include "../components/interactive/led.h"
#include "../components/interactive/seven_segment.h"
#include "../components/digital/logic_gates.h"
#include "../components/digital/d_flipflop.h"
#include "../measurement/voltmeter.h"
#include "../measurement/ammeter.h"

Component* buildComponent(const vector<string>& t) {
    char tag = t[0][0];
    string name = t[1];
    double x = stod(t[2]);
    double y = stod(t[3]);
    Component* c = nullptr;

    if (tag == 'R')
        c = new Resistor(name, x, y, stod(t[7]));
    else if (tag == 'C') {
        c = new Capacitor(name, x, y, stod(t[7]));
        ((Capacitor*)c)->lastVoltage = stod(t[8]);
    } else if (tag == 'L') {
        c = new Inductor(name, x, y, stod(t[7]));
        ((Inductor*)c)->lastCurrent = stod(t[8]);
    } else if (tag == 'G')
        c = new Ground(name, x, y);
    else if (tag == 'V')
        c = new DCSource(name, x, y, stod(t[7]));
    else if (tag == 'B') {
        c = new Battery(name, x, y, stod(t[7]));
        ((Battery*)c)->internalResistance = stod(t[8]);
    } else if (tag == 'K') {
        c = new ClockGenerator(name, x, y, stod(t[7]));
        ((ClockGenerator*)c)->highVoltage = stod(t[8]);
    } else if (tag == 'S') {
        c = new Switch(name, x, y);
        ((Switch*)c)->closed = t[7] == "1";
    } else if (tag == 'P') {
        c = new PushButton(name, x, y);
        ((PushButton*)c)->pressed = t[7] == "1";
    } else if (tag == 'D') {
        c = new LED(name, x, y, t[8]);
        ((LED*)c)->threshold = stod(t[7]);
    } else if (tag == 'E')
        c = new SevenSegment(name, x, y);
    else if (tag == 'A')
        c = new AndGate(name, x, y, stoi(t[7]));
    else if (tag == 'O')
        c = new OrGate(name, x, y, stoi(t[7]));
    else if (tag == 'N')
        c = new NotGate(name, x, y);
    else if (tag == 'X')
        c = new XorGate(name, x, y, stoi(t[7]));
    else if (tag == 'Q')
        c = new NandGate(name, x, y, stoi(t[7]));
    else if (tag == 'F') {
        c = new DFlipFlop(name, x, y);
        ((DFlipFlop*)c)->stored = (LogicLevel)stoi(t[7]);
        ((DFlipFlop*)c)->lastClk = (LogicLevel)stoi(t[8]);
    } else if (tag == 'M')
        c = new Voltmeter(name, x, y);
    else if (tag == 'Y')
        c = new Ammeter(name, x, y);

    if (c) {
        c->rotation = stoi(t[4]);
        c->mirroredH = t[5] == "1";
        c->mirroredV = t[6] == "1";
        if (tag == 'A' || tag == 'O' || tag == 'X' || tag == 'Q')
            ((LogicGate*)c)->propagationDelay = stod(t[8]);
        if (tag == 'N')
            ((LogicGate*)c)->propagationDelay = stod(t[7]);
    }
    return c;
}