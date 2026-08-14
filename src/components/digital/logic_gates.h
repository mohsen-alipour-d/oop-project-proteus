#pragma once

#include "logic_gate.h"

using namespace std;

class AndGate : public LogicGate {
public:
    AndGate() {}
    AndGate(string name, double x, double y, int inputCount);
    LogicLevel apply(vector<LogicLevel>& ins) override;

    string typeTag() const override { return "A"; }
    string extraData() const override { return to_string((int)pins.size() - 1) + " " + to_string(propagationDelay); }
};

class OrGate : public LogicGate {
public:
    OrGate() {}
    OrGate(string name, double x, double y, int inputCount);
    LogicLevel apply(vector<LogicLevel>& ins) override;

    string typeTag() const override { return "O"; }
    string extraData() const override { return to_string((int)pins.size() - 1) + " " + to_string(propagationDelay); }
};

class NotGate : public LogicGate {
public:
    NotGate() {}
    NotGate(string name, double x, double y);
    LogicLevel apply(vector<LogicLevel>& ins) override;

    string typeTag() const override { return "N"; }
    string extraData() const override { return to_string(propagationDelay); }
};

class XorGate : public LogicGate {
public:
    XorGate() {}
    XorGate(string name, double x, double y, int inputCount);
    LogicLevel apply(vector<LogicLevel>& ins) override;

    string typeTag() const override { return "X"; }
    string extraData() const override { return to_string((int)pins.size() - 1) + " " + to_string(propagationDelay); }
};

class NandGate : public LogicGate {
public:
    NandGate() {}
    NandGate(string name, double x, double y, int inputCount);
    LogicLevel apply(vector<LogicLevel>& ins) override;

    string typeTag() const override { return "Q"; }
    string extraData() const override { return to_string((int)pins.size() - 1) + " " + to_string(propagationDelay); }
};