#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../core/component.h"

using namespace std;

class ExternalMemory : public Component {
public:
    ExternalMemory() {}
    ExternalMemory(string name, double x, double y, int addressBits = 8);

    void step(double dt, double simTime) override;

    int getAddressBits() const { return addressBits; }
    size_t size() const { return memory.size(); }
    uint8_t readByte(size_t address) const;
    void writeByte(size_t address, uint8_t value);

    int addressPinIndex(int bit) const { return bit; }
    int dataPinIndex(int bit) const { return addressBits + bit; }
    int readPinIndex() const { return addressBits + 8; }
    int writePinIndex() const { return addressBits + 9; }

    string typeTag() const override { return "MEM"; }
    string extraData() const override;

    string exportMemoryHex() const;
    bool importMemoryHex(const string& hex);

private:
    int addressBits = 8;
    vector<uint8_t> memory;

    size_t readAddressBus() const;
    uint8_t readDataBus() const;
    void driveDataBus(uint8_t value, bool enabled);
};
