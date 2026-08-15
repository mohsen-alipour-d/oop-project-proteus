#include <iostream>
#include <cmath>

#include "../src/core/circuit.h"
#include "../src/components/sources/ground.h"
#include "../src/components/sources/dc_source.h"
#include "../src/components/passive/resistor.h"
#include "../src/components/digital/logic_gates.h"
#include "../src/components/digital/d_flipflop.h"
#include "../src/file/file_manager.h"
#include "../src/file/history.h"
#include "../src/drc/drc.h"

using namespace std;

static int passed = 0;
static int failed = 0;

static void check(bool cond, const string& name) {
    if (cond) {
        passed++;
        cout << "PASS " << name << endl;
    } else {
        failed++;
        cout << "FAIL " << name << endl;
    }
}

static bool near(double a, double b) {
    return fabs(a - b) < 0.0001;
}

static void testWireRouting() {
    Circuit c;
    c.addComponent(new Resistor("R1", 0, 0, 100));
    c.addComponent(new Resistor("R2", 40, 30, 100));
    Wire* w = c.addWire(&c.components[0]->pins[0], &c.components[1]->pins[0]);
    check(w->points.size() == 3, "wire has 3 points for 90 degree route");
    check(near(w->points[1].x, w->points[2].x), "middle point shares x with end");
}

static void testMoveComponent() {
    Circuit c;
    c.addComponent(new DCSource("V1", 0, 0, 5));
    c.addComponent(new Resistor("R1", 60, 0, 100));
    Wire* w = c.addWire(&c.components[0]->pins[0], &c.components[1]->pins[0]);
    double oldEndX = w->points.back().x;
    c.moveComponent(c.components[1], 20, 0);
    check(near(w->points.back().x, oldEndX + 20), "wire endpoint follows moved component");
}

static void testUIHelpers() {
    Circuit c;
    Component* r = new Resistor("R1", 53, 47, 100);
    c.addComponent(r);
    r->snapToGrid(10);
    check(near(r->position.x, 50) && near(r->position.y, 50), "snap to grid rounds position");
    Rect box = r->getBoundingBox();
    check(box.contains(Vector2D(50, 50)), "bounding box contains component center");
    check(c.getComponentsInRect(box).size() == 1, "rect selection finds component");
}

static void testSerialize() {
    Circuit c;
    c.addComponent(new Ground("G1", 0, 0));
    c.addComponent(new DCSource("V1", 40, 0, 5));
    c.addComponent(new Resistor("R1", 80, 0, 220));
    c.addWire(&c.components[1]->pins[0], &c.components[2]->pins[0]);
    FileManager fm;
    Circuit loaded;
    fm.deserialize(loaded, fm.serialize(c));
    check(loaded.components.size() == 3, "deserialize restores component count");
    check(loaded.wires.size() == 1, "deserialize restores wire count");
    check(near(((Resistor*)loaded.components[2])->value, 220), "deserialize restores resistor value");
}

static void testUndoRedo() {
    Circuit c;
    c.addComponent(new Resistor("R1", 0, 0, 100));
    FileManager fm;
    History h;
    h.push(fm.serialize(c));
    ((Resistor*)c.components[0])->value = 330;
    h.push(fm.serialize(c));
    fm.deserialize(c, h.undo());
    check(near(((Resistor*)c.components[0])->value, 100), "undo restores old value");
    fm.deserialize(c, h.redo());
    check(near(((Resistor*)c.components[0])->value, 330), "redo restores new value");
}

static void testGates() {
    NotGate not1("N1", 0, 0);
    not1.propagationDelay = 0;
    not1.pins[0].connected = true;
    not1.pins[0].voltage = 5;
    not1.step(0.01, 0);
    check(near(not1.pins[1].voltage, 0), "NOT inverts HIGH to LOW");

    AndGate and1("A1", 0, 0, 2);
    and1.propagationDelay = 0;
    and1.pins[0].connected = true;
    and1.pins[1].connected = true;
    and1.pins[0].voltage = 5;
    and1.pins[1].voltage = 5;
    and1.step(0.01, 0);
    check(near(and1.pins[2].voltage, 5), "AND outputs HIGH when both HIGH");
}

static void testFlipFlop() {
    DFlipFlop ff("F1", 0, 0);
    ff.pins[0].connected = true;
    ff.pins[1].connected = true;
    ff.pins[0].voltage = 5;
    ff.pins[1].voltage = 0;
    ff.step(0.01, 0);
    ff.pins[1].voltage = 5;
    ff.step(0.01, 0.01);
    check(near(ff.pins[2].voltage, 5), "D-FF captures D on rising edge");
}

static void testDRC() {
    Circuit floating;
    floating.addComponent(new AndGate("A1", 0, 0, 2));
    DRC d1;
    check(!d1.checkFloating(floating), "floating input detected");

    Circuit shorted;
    shorted.addComponent(new Ground("G1", 0, 0));
    shorted.addComponent(new DCSource("V1", 40, 0, 5));
    shorted.addWire(&shorted.components[0]->pins[0], &shorted.components[1]->pins[0]);
    shorted.components[0]->pins[0].voltage = 0;
    shorted.components[1]->pins[0].voltage = 5;
    DRC d2;
    check(!d2.checkShorts(shorted), "short circuit detected");
}

int main() {
    testWireRouting();
    testMoveComponent();
    testUIHelpers();
    testSerialize();
    testUndoRedo();
    testGates();
    testFlipFlop();
    testDRC();
    cout << "\n" << passed << " passed, " << failed << " failed" << endl;
    return failed == 0 ? 0 : 1;
}