#pragma once

#include "../core/circuit.h"
#include "../core/vector2d.h"

using namespace std;

class VoltageProbe {
public:
    double read(Circuit& circuit, const Vector2D& pos);
    bool isFloating(Circuit& circuit, const Vector2D& pos);
};