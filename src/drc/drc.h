#pragma once

#include "../core/circuit.h"
#include "log_buffer.h"

using namespace std;

class DRC {
public:
    LogBuffer log;

    DRC() {}

    bool checkShorts(Circuit& c);
    bool checkFloating(Circuit& c);
    bool validate(Circuit& c);
};