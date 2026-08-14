#include "net.h"

Net::Net(int id) {
    this->id = id;
}

void Net::addPin(Pin* p) {
    pins.push_back(p);
}

bool Net::hasPin(Pin* p) const {
    for (Pin* q : pins)
        if (q == p)
            return true;
    return false;
}