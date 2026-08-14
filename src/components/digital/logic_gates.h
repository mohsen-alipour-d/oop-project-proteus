#pragma once

#include "logic_gate.h"

using namespace std;

class AndGate : public LogicGate {
public:
    AndGate() {}
    AndGate(string name, double x, double y, int inputCount);
    LogicLevel apply(vector<LogicLevel>& ins) override;
};

class OrGate : public LogicGate {
public:
    OrGate() {}
    OrGate(string name, double x, double y, int inputCount);
    LogicLevel apply(vector<LogicLevel>& ins) override;
};

class NotGate : public LogicGate {
public:
    NotGate() {}
    NotGate(string name, double x, double y);
    LogicLevel apply(vector<LogicLevel>& ins) override;
};

class XorGate : public LogicGate {
public:
    XorGate() {}
    XorGate(string name, double x, double y, int inputCount);
    LogicLevel apply(vector<LogicLevel>& ins) override;
};

class NandGate : public LogicGate {
public:
    NandGate() {}
    NandGate(string name, double x, double y, int inputCount);
    LogicLevel apply(vector<LogicLevel>& ins) override;
};