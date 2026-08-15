#pragma once

#include <cstdint>
#include <string>

#include "../../core/component.h"
#include "../../mcu/program_counter.h"
#include "../../mcu/register_file.h"
#include "../../mcu/internal_ram.h"
#include "../../mcu/flash_memory.h"
#include "../../mcu/mcu_port.h"
#include "../../mcu/instruction_decoder.h"

using namespace std;

class Microcontroller : public Component {
public:
    static constexpr int PORT_A = 0;
    static constexpr int PORT_B = 1;

    Microcontroller() {}
    Microcontroller(string name, double x, double y, double instructionPeriod = 0.001);

    void step(double dt, double simTime) override;
    bool executeOneInstruction();
    void reset();

    bool loadFirmware(const string& path);
    const string& lastError() const { return error; }

    ProgramCounter& pc() { return programCounter; }
    const ProgramCounter& pc() const { return programCounter; }
    RegisterFile& registers() { return registerFile; }
    const RegisterFile& registers() const { return registerFile; }
    InternalRAM& ram() { return internalRam; }
    const InternalRAM& ram() const { return internalRam; }
    FlashMemory& flash() { return flashMemory; }
    const FlashMemory& flash() const { return flashMemory; }
    MCUPort& port(int index);
    const MCUPort& port(int index) const;

    double getInstructionPeriod() const { return instructionPeriod; }
    void setInstructionPeriod(double period);
    bool isHalted() const { return halted; }
    void halt(const string& reason);

    int portPinIndex(int portIndex, int bit) const { return portIndex * 8 + bit; }

    string typeTag() const override { return "MCU"; }
    string extraData() const override;

private:
    ProgramCounter programCounter;
    RegisterFile registerFile;
    InternalRAM internalRam;
    FlashMemory flashMemory;
    MCUPort portA{"A"};
    MCUPort portB{"B"};
    InstructionDecoder decoder;

    double instructionPeriod = 0.001;
    double nextInstructionTime = 0.0;
    bool halted = false;
    string error;

    void bindPorts();
};
