#include "external_memory.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "../digital/logic_level.h"

ExternalMemory::ExternalMemory(string name, double x, double y, int addressBits)
    : Component(name, x, y) {
    if (addressBits < 1 || addressBits > 16)
        throw invalid_argument("External memory address width must be 1..16 bits");
    this->addressBits = addressBits;
    memory.assign(size_t(1) << addressBits, 0);

    for (int bit = 0; bit < addressBits; ++bit)
        addPin(-30, bit * 8 - (addressBits - 1) * 4); // address input
    for (int bit = 0; bit < 8; ++bit) {
        addPin(30, bit * 10 - 35); // bidirectional data bus
        pins.back().setDirection(PinDirection::Bidirectional);
    }
    addPin(-10, -25); // RD, active high
    addPin(-10, 25);  // WR, active high
}

uint8_t ExternalMemory::readByte(size_t address) const {
    if (address >= memory.size())
        throw out_of_range("External memory address out of range");
    return memory[address];
}

void ExternalMemory::writeByte(size_t address, uint8_t value) {
    if (address >= memory.size())
        throw out_of_range("External memory address out of range");
    memory[address] = value;
}

size_t ExternalMemory::readAddressBus() const {
    size_t address = 0;
    for (int bit = 0; bit < addressBits; ++bit)
        if (voltageToLogic(pins[addressPinIndex(bit)].voltage) == HIGH)
            address |= size_t(1) << bit;
    return address;
}

uint8_t ExternalMemory::readDataBus() const {
    uint8_t value = 0;
    for (int bit = 0; bit < 8; ++bit)
        if (voltageToLogic(pins[dataPinIndex(bit)].voltage) == HIGH)
            value |= static_cast<uint8_t>(1u << bit);
    return value;
}

void ExternalMemory::driveDataBus(uint8_t value, bool enabled) {
    for (int bit = 0; bit < 8; ++bit) {
        Pin& pin = pins[dataPinIndex(bit)];
        pin.setDriving(enabled);
        if (enabled)
            pin.voltage = ((value >> bit) & 1u) ? 5.0 : 0.0;
    }
}

void ExternalMemory::step(double dt, double simTime) {
    (void)dt;
    (void)simTime;
    const bool readEnabled = voltageToLogic(pins[readPinIndex()].voltage) == HIGH;
    const bool writeEnabled = voltageToLogic(pins[writePinIndex()].voltage) == HIGH;
    const size_t address = readAddressBus();

    if (writeEnabled) {
        driveDataBus(0, false);
        writeByte(address, readDataBus());
    } else if (readEnabled) {
        driveDataBus(readByte(address), true);
    } else {
        driveDataBus(0, false);
    }
}

string ExternalMemory::exportMemoryHex() const {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0');
    for (uint8_t b : memory)
        out << std::setw(2) << static_cast<int>(b);
    return out.str();
}

bool ExternalMemory::importMemoryHex(const string& hex) {
    if (hex.size() != memory.size() * 2)
        return false;
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return -1;
    };
    for (size_t i = 0; i < memory.size(); ++i) {
        int hi = nibble(hex[2 * i]);
        int lo = nibble(hex[2 * i + 1]);
        if (hi < 0 || lo < 0)
            return false;
        memory[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return true;
}

string ExternalMemory::extraData() const {
    return to_string(addressBits) + " " + exportMemoryHex();
}
