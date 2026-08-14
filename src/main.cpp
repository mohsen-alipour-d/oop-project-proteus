#include <iostream>

#include "core/component.h"
#include "components/interactive/switch_comp.h"
#include "components/interactive/push_button.h"
#include "components/interactive/led.h"
#include "components/interactive/seven_segment.h"

using namespace std;

int main() {
    Switch sw("SW1", 0, 0);
    sw.pins[0].voltage = 5;
    sw.step(0.01, 0);
    cout << "switch open out " << sw.pins[1].voltage << endl;
    sw.toggle();
    sw.step(0.01, 0);
    cout << "switch closed out " << sw.pins[1].voltage << endl;

    PushButton pb("PB1", 40, 0);
    pb.step(0.01, 0);
    cout << "button released " << pb.pins[0].voltage << endl;
    pb.press();
    pb.step(0.01, 0);
    cout << "button pressed " << pb.pins[0].voltage << endl;

    LED led("LED1", 80, 0, "green");
    led.pins[0].voltage = 5;
    led.pins[1].voltage = 0;
    led.step(0.01, 0);
    cout << "led lit " << led.lit << endl;

    SevenSegment seg("SEG1", 120, 0);
    seg.pins[0].voltage = 5;
    seg.pins[3].voltage = 5;
    seg.step(0.01, 0);
    cout << "seg0 " << seg.segOn[0] << " seg1 " << seg.segOn[1] << " seg3 " << seg.segOn[3] << endl;

    return 0;
}