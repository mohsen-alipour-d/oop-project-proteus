#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "../../core/component.h"

using namespace std;

class LCD16x2 : public Component {
public:
    static constexpr int DATA0_PIN = 0;
    static constexpr int RS_PIN = 8;
    static constexpr int RW_PIN = 9;
    static constexpr int ENABLE_PIN = 10;

    LCD16x2() {}
    LCD16x2(string name, double x, double y);

    void step(double dt, double simTime) override;

    void clear();
    void home();
    void setCursor(int row, int col);
    void writeChar(char value);
    string line(int row) const;
    string line1() const { return line(0); }
    string line2() const { return line(1); }

    int dataPinIndex(int bit) const { return bit; }

    string typeTag() const override { return "LCD"; }

private:
    array<array<char, 16>, 2> display{};
    int cursorRow = 0;
    int cursorCol = 0;
    bool lastEnable = false;

    uint8_t readDataByte() const;
    void executeCommand(uint8_t command);
};
