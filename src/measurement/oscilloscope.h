#pragma once

#include <vector>

#include "../core/circuit.h"

using namespace std;

struct ScopeChannel {
    int netId = -1;
    vector<double> history;
    double timeDiv = 1.0;
    double voltDiv = 1.0;
};

class Oscilloscope {
public:
    vector<ScopeChannel> channels;
    double currentTime = 0.0;
    bool isRunning = false;

    Oscilloscope();

    void attachChannel(int chIndex, int netId);
    void start();
    void update(Circuit& circuit, double simTime);
    void pause();
    void stop();
};
