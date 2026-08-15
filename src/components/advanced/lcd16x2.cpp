#include "lcd16x2.h"

#include <algorithm>
#include <stdexcept>

#include "../digital/logic_level.h"

LCD16x2::LCD16x2(string name, double x, double y) : Component(name, x, y) {
    for (int bit = 0; bit < 8; ++bit)
        addPin(-30, bit * 8 - 28); // D0..D7
    addPin(-10, -20); // RS
    addPin(-10, 0);   // RW
    addPin(-10, 20);  // E
    clear();
}

void LCD16x2::clear() {
    for (auto& row : display)
        row.fill(' ');
    cursorRow = 0;
    cursorCol = 0;
}

void LCD16x2::home() {
    cursorRow = 0;
    cursorCol = 0;
}

void LCD16x2::setCursor(int row, int col) {
    cursorRow = (row < 0) ? 0 : ((row > 1) ? 1 : row);
    cursorCol = (col < 0) ? 0 : ((col > 15) ? 15 : col);
}

void LCD16x2::writeChar(char value) {
    display[cursorRow][cursorCol] = value;
    ++cursorCol;
    if (cursorCol >= 16) {
        cursorCol = 0;
        cursorRow = (cursorRow + 1) % 2;
    }
}

string LCD16x2::line(int row) const {
    if (row < 0 || row > 1)
        throw out_of_range("LCD row out of range");
    return string(display[row].begin(), display[row].end());
}

uint8_t LCD16x2::readDataByte() const {
    uint8_t value = 0;
    for (int bit = 0; bit < 8; ++bit)
        if (voltageToLogic(pins[dataPinIndex(bit)].voltage) == HIGH)
            value |= static_cast<uint8_t>(1u << bit);
    return value;
}

void LCD16x2::executeCommand(uint8_t command) {
    if (command == 0x01) {
        clear();
        return;
    }
    if (command == 0x02) {
        home();
        return;
    }
    if (command & 0x80u) {
        uint8_t address = command & 0x7Fu;
        if (address >= 0x40)
            setCursor(1, address - 0x40);
        else
            setCursor(0, address);
    }
}

void LCD16x2::step(double dt, double simTime) {
    (void)dt;
    (void)simTime;
    const bool enable = voltageToLogic(pins[ENABLE_PIN].voltage) == HIGH;
    const bool rising = !lastEnable && enable;
    lastEnable = enable;
    if (!rising)
        return;

    const bool readMode = voltageToLogic(pins[RW_PIN].voltage) == HIGH;
    if (readMode)
        return; // Readback is outside the required simplified LCD behavior.

    const uint8_t byte = readDataByte();
    const bool dataMode = voltageToLogic(pins[RS_PIN].voltage) == HIGH;
    if (dataMode)
        writeChar(static_cast<char>(byte));
    else
        executeCommand(byte);
}
