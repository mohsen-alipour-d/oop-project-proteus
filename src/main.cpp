#include <iostream>

#include "core/component.h"
#include "components/sources/ground.h"
#include "components/sources/dc_source.h"
#include "components/sources/battery.h"
#include "components/sources/clock_gen.h"

using namespace std;

int main() {
    Ground g("GND1", 0, 0);
    DCSource src("V1", 0, -20, 5);
    Battery bat("B1", 40, 0, 9);
    ClockGenerator clk("CLK1", 80, 0, 1.0);

    src.pins[1].voltage = 0;

    g.step(0.01, 0);
    src.step(0.01, 0);
    bat.step(0.01, 0);

    cout << "gnd " << g.pins[0].voltage << endl;
    cout << "src+ " << src.pins[0].voltage << endl;
    cout << "bat+ " << bat.pins[0].voltage << endl;

    for (int i = 0; i < 4; i++) {
        double t = i * 0.25;
        clk.step(0.01, t);
        cout << "clk at " << t << " -> " << clk.pins[0].voltage << endl;
    }

    return 0;
}