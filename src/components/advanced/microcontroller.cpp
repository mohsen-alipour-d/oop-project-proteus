#include "microcontroller.h"

#include <array>
#include <stdexcept>

#include "../../mcu/firmware_loader.h"

Microcontroller::Microcontroller(string name, double x, double y, double instructionPeriod)
    : Component(name, x, y), internalRam(256), flashMemory(65536) {
    setInstructionPeriod(instructionPeriod);

    for (int bit = 0; bit < 8; ++bit)
        addPin(-30, bit * 10 - 35); // Port A
    for (int bit = 0; bit < 8; ++bit)
        addPin(30, bit * 10 - 35); // Port B

    bindPorts();
    reset();
}

void Microcontroller::bindPorts() {
    std::array<Pin*, 8> a{};
    std::array<Pin*, 8> b{};
    for (int bit = 0; bit < 8; ++bit) {
        a[bit] = &pins[portPinIndex(PORT_A, bit)];
        b[bit] = &pins[portPinIndex(PORT_B, bit)];
    }
    portA.bind(a);
    portB.bind(b);
}

void Microcontroller::setInstructionPeriod(double period) {
    if (period <= 0.0)
        throw invalid_argument("MCU instruction period must be positive");
    instructionPeriod = period;
}

MCUPort& Microcontroller::port(int index) {
    if (index == PORT_A) return portA;
    if (index == PORT_B) return portB;
    throw out_of_range("MCU port index out of range");
}

const MCUPort& Microcontroller::port(int index) const {
    if (index == PORT_A) return portA;
    if (index == PORT_B) return portB;
    throw out_of_range("MCU port index out of range");
}

void Microcontroller::reset() {
    programCounter.reset();
    registerFile.reset();
    internalRam.clear();
    portA.reset();
    portB.reset();
    nextInstructionTime = 0.0;
    halted = false;
    error.clear();
}

bool Microcontroller::loadFirmware(const string& path) {
    FirmwareLoader loader;
    if (!loader.loadHex(path, flashMemory)) {
        error = loader.lastError();
        return false;
    }
    reset();
    return true;
}

void Microcontroller::halt(const string& reason) {
    halted = true;
    error = reason;
}

bool Microcontroller::executeOneInstruction() {
    if (halted)
        return false;
    if (!decoder.executeNext(*this)) {
        halt(decoder.lastError());
        return false;
    }
    return true;
}

void Microcontroller::step(double dt, double simTime) {
    (void)dt;
    if (halted)
        return;

    int guard = 0;
    while (simTime + 1e-12 >= nextInstructionTime && !halted && guard < 10000) {
        executeOneInstruction();
        nextInstructionTime += instructionPeriod;
        ++guard;
    }
}

string Microcontroller::extraData() const {
    return to_string(instructionPeriod) + " " +
           to_string(static_cast<int>(portA.directionMask())) + " " +
           to_string(static_cast<int>(portB.directionMask())) + " " +
           flashMemory.exportHex();
}
