#include "logic_level.h"

LogicLevel voltageToLogic(double v) {
    if (v <= LOW_MAX)
        return LOW;
    if (v >= HIGH_MIN)
        return HIGH;
    return UNDEFINED;
}

double logicToVoltage(LogicLevel l) {
    if (l == HIGH)
        return HIGH_VOLT;
    if (l == LOW)
        return LOW_VOLT;
    return UNDEFINED_VOLT;
}