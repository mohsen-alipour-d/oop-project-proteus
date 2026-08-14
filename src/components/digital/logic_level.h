#pragma once

using namespace std;

enum LogicLevel {
    LOW,
    HIGH,
    UNDEFINED
};

const double LOW_VOLT = 0.0;
const double HIGH_VOLT = 5.0;
const double UNDEFINED_VOLT = 1.4;
const double LOW_MAX = 0.8;
const double HIGH_MIN = 2.0;

LogicLevel voltageToLogic(double v);
double logicToVoltage(LogicLevel l);