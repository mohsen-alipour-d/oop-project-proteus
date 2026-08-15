#pragma once

#include <array>
#include <string>

#include "../../core/component.h"

using namespace std;

class MatrixKeypad : public Component {
public:
    MatrixKeypad() {}
    MatrixKeypad(string name, double x, double y);

    void step(double dt, double simTime) override;

    void press(int row, int col);
    void release(int row, int col);
    void releaseAll();
    bool isPressed(int row, int col) const;
    char keyLabel(int row, int col) const;

    int rowPinIndex(int row) const { return row; }
    int colPinIndex(int col) const { return 4 + col; }

    string typeTag() const override { return "KEYPAD"; }

private:
    array<array<bool, 4>, 4> pressed{};
};
