#include "logic_gates.h"

AndGate::AndGate(string name, double x, double y, int inputCount) : LogicGate(name, x, y, inputCount) {
}

LogicLevel AndGate::apply(vector<LogicLevel>& ins) {
    for (LogicLevel l : ins)
        if (l == LOW)
            return LOW;
    return HIGH;
}

OrGate::OrGate(string name, double x, double y, int inputCount) : LogicGate(name, x, y, inputCount) {
}

LogicLevel OrGate::apply(vector<LogicLevel>& ins) {
    for (LogicLevel l : ins)
        if (l == HIGH)
            return HIGH;
    return LOW;
}

NotGate::NotGate(string name, double x, double y) : LogicGate(name, x, y, 1) {
}

LogicLevel NotGate::apply(vector<LogicLevel>& ins) {
    return ins[0] == HIGH ? LOW : HIGH;
}

XorGate::XorGate(string name, double x, double y, int inputCount) : LogicGate(name, x, y, inputCount) {
}

LogicLevel XorGate::apply(vector<LogicLevel>& ins) {
    int highs = 0;
    for (LogicLevel l : ins)
        if (l == HIGH)
            highs++;
    return highs % 2 == 1 ? HIGH : LOW;
}

NandGate::NandGate(string name, double x, double y, int inputCount) : LogicGate(name, x, y, inputCount) {
}

LogicLevel NandGate::apply(vector<LogicLevel>& ins) {
    for (LogicLevel l : ins)
        if (l == LOW)
            return HIGH;
    return LOW;
}