#include "oscilloscope.h"

Oscilloscope::Oscilloscope() {
    channels.resize(2);
}

void Oscilloscope::attachChannel(int chIndex, int netId) {
    if (chIndex >= 0 && chIndex < static_cast<int>(channels.size())) {
        channels[chIndex].netId = netId;
        channels[chIndex].history.clear();
    }
}

void Oscilloscope::start() {
    isRunning = true;
}

void Oscilloscope::update(Circuit& circuit, double simTime) {
    if (!isRunning)
        return;
    currentTime = simTime;
    for (int i = 0; i < static_cast<int>(channels.size()); i++) {
        double v = 0.0;
        if (channels[i].netId >= 0 &&
            channels[i].netId < static_cast<int>(circuit.nets.size())) {
            v = circuit.nets[channels[i].netId]->voltage;
        }
        channels[i].history.push_back(v);
        if (channels[i].history.size() > 2000)
            channels[i].history.erase(channels[i].history.begin(),
                                      channels[i].history.begin() + 500);
    }
}

void Oscilloscope::pause() {
    isRunning = false;
}

void Oscilloscope::stop() {
    isRunning = false;
    currentTime = 0.0;
    for (int i = 0; i < static_cast<int>(channels.size()); i++)
        channels[i].history.clear();
}
