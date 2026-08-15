#include "matrix_keypad.h"

#include <stdexcept>

#include "../digital/logic_level.h"

MatrixKeypad::MatrixKeypad(string name, double x, double y) : Component(name, x, y) {
    for (int row = 0; row < 4; ++row) {
        addPin(-25, row * 12 - 18);
        pins.back().setDirection(PinDirection::Bidirectional);
    }
    for (int col = 0; col < 4; ++col)
        addPin(25, col * 12 - 18); // columns are scanned/driven externally
    releaseAll();
}

void MatrixKeypad::press(int row, int col) {
    if (row < 0 || row > 3 || col < 0 || col > 3)
        throw out_of_range("Keypad position out of range");
    pressed[row][col] = true;
}

void MatrixKeypad::release(int row, int col) {
    if (row < 0 || row > 3 || col < 0 || col > 3)
        throw out_of_range("Keypad position out of range");
    pressed[row][col] = false;
}

void MatrixKeypad::releaseAll() {
    for (auto& row : pressed)
        row.fill(false);
}

bool MatrixKeypad::isPressed(int row, int col) const {
    if (row < 0 || row > 3 || col < 0 || col > 3)
        throw out_of_range("Keypad position out of range");
    return pressed[row][col];
}

char MatrixKeypad::keyLabel(int row, int col) const {
    static const char labels[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    if (row < 0 || row > 3 || col < 0 || col > 3)
        throw out_of_range("Keypad position out of range");
    return labels[row][col];
}

void MatrixKeypad::step(double dt, double simTime) {
    (void)dt;
    (void)simTime;

    for (int row = 0; row < 4; ++row) {
        Pin& rowPin = pins[rowPinIndex(row)];
        rowPin.setDriving(false);

        for (int col = 0; col < 4; ++col) {
            if (!pressed[row][col])
                continue;
            LogicLevel columnLevel = voltageToLogic(pins[colPinIndex(col)].voltage);
            if (columnLevel == HIGH || columnLevel == LOW) {
                rowPin.voltage = columnLevel == HIGH ? 5.0 : 0.0;
                rowPin.setDriving(true);
                break;
            }
        }
    }
}
