#include "seven_segment.h"

SevenSegment::SevenSegment(string name, double x, double y) : Component(name, x, y) {
    for (int i = 0; i < 8; i++)
        addPin(0, i * 5 - 15);
}

void SevenSegment::step(double dt, double simTime) {
    for (int i = 0; i < 8; i++)
        segOn[i] = pins[i].voltage >= logicHigh;
}