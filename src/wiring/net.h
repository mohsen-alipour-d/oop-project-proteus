#pragma once

#include <vector>

#include "../core/pin.h"

using namespace std;

class Net {
public:
    int id = -1;
    vector<Pin*> pins;
    double voltage = 0;

    Net() {}
    Net(int id);

    void addPin(Pin* p);
    bool hasPin(Pin* p) const;
};