#include <iostream>

#include "core/component.h"

using namespace std;

int main() {
    Component res("R1", 100, 100);
    res.addPin(20, 0);
    res.addPin(-20, 0);

    cout << "pin at " << res.pins[0].worldPos().x << "," << res.pins[0].worldPos().y << endl;

    Vector2D mouse(121, 101);
    if (res.pins[0].checkMouseOver(mouse))
        cout << "mouse over pin" << endl;

    res.rotate90();
    cout << "after rotate pin0 at " << res.pins[0].worldPos().x << "," << res.pins[0].worldPos().y << endl;

    return 0;
}