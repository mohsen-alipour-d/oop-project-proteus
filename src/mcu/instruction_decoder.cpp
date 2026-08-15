#include "instruction_decoder.h"

#include <cstdint>
#include <sstream>

#include "../components/advanced/microcontroller.h"

bool InstructionDecoder::fail(const std::string& message) {
    error = message;
    return false;
}

bool InstructionDecoder::executeNext(Microcontroller& mcu) {
    error.clear();
    const uint32_t pc = mcu.pc().value();
    if (pc >= mcu.flash().size())
        return fail("Program counter moved outside Flash memory");

    const uint8_t op = mcu.flash().read(pc);

    auto byteAt = [&](uint32_t offset, uint8_t& out) -> bool {
        uint32_t addr = pc + offset;
        if (addr >= mcu.flash().size())
            return false;
        out = mcu.flash().read(addr);
        return true;
    };

    // Educational ISA used by this simulator.
    // 10 reg imm        MOV reg, #imm       (reg 0=A, 1..8=R0..R7)
    // 11 dst src        MOV dst, src
    // 12 reg port       MOV reg, PORT       (port 0=A, 1=B)
    // 13 port reg       MOV PORT, reg
    // 20 reg imm        ADD reg, #imm
    // 30 hi lo          JMP absolute16
    // 40 port bit       SETB port.bit
    // 50 port bit       CLR port.bit
    if (op == 0x10) {
        uint8_t reg = 0, imm = 0;
        if (!byteAt(1, reg) || !byteAt(2, imm)) return fail("Truncated MOV instruction");
        if (reg > 8) return fail("Invalid MOV register index");
        mcu.registers().set(reg, imm);
        mcu.pc().advance(3);
        return true;
    }
    if (op == 0x11) {
        uint8_t dst = 0, src = 0;
        if (!byteAt(1, dst) || !byteAt(2, src)) return fail("Truncated MOV register instruction");
        if (dst > 8 || src > 8) return fail("Invalid MOV register index");
        mcu.registers().set(dst, mcu.registers().get(src));
        mcu.pc().advance(3);
        return true;
    }
    if (op == 0x12) {
        uint8_t reg = 0, port = 0;
        if (!byteAt(1, reg) || !byteAt(2, port)) return fail("Truncated MOV port-read instruction");
        if (reg > 8 || port > 1) return fail("Invalid MOV port-read operand");
        mcu.registers().set(reg, mcu.port(port).read());
        mcu.pc().advance(3);
        return true;
    }
    if (op == 0x13) {
        uint8_t port = 0, reg = 0;
        if (!byteAt(1, port) || !byteAt(2, reg)) return fail("Truncated MOV port-write instruction");
        if (port > 1 || reg > 8) return fail("Invalid MOV port-write operand");
        mcu.port(port).write(mcu.registers().get(reg));
        mcu.pc().advance(3);
        return true;
    }
    if (op == 0x20) {
        uint8_t reg = 0, imm = 0;
        if (!byteAt(1, reg) || !byteAt(2, imm)) return fail("Truncated ADD instruction");
        if (reg > 8) return fail("Invalid ADD register index");
        mcu.registers().set(reg, static_cast<uint8_t>(mcu.registers().get(reg) + imm));
        mcu.pc().advance(3);
        return true;
    }
    if (op == 0x30) {
        uint8_t hi = 0, lo = 0;
        if (!byteAt(1, hi) || !byteAt(2, lo)) return fail("Truncated JMP instruction");
        mcu.pc().jump(static_cast<uint16_t>((hi << 8) | lo));
        return true;
    }
    if (op == 0x40 || op == 0x50) {
        uint8_t port = 0, bit = 0;
        if (!byteAt(1, port) || !byteAt(2, bit)) return fail("Truncated bit instruction");
        if (port > 1 || bit > 7) return fail("Invalid port bit operand");
        if (op == 0x40) mcu.port(port).setBit(bit);
        else mcu.port(port).clearBit(bit);
        mcu.pc().advance(3);
        return true;
    }

    // Small 8051-compatible subset, useful when firmware is generated in a
    // familiar assembly form. The simulator still intentionally models only
    // the five operations required by the assignment.
    if (op == 0x74) { // MOV A,#data
        uint8_t imm = 0;
        if (!byteAt(1, imm)) return fail("Truncated 8051 MOV A,#data");
        mcu.registers().setAccumulator(imm);
        mcu.pc().advance(2);
        return true;
    }
    if (op >= 0x78 && op <= 0x7F) { // MOV Rn,#data
        uint8_t imm = 0;
        if (!byteAt(1, imm)) return fail("Truncated 8051 MOV Rn,#data");
        int reg = 1 + (op - 0x78);
        mcu.registers().set(reg, imm);
        mcu.pc().advance(2);
        return true;
    }
    if (op == 0x24) { // ADD A,#data
        uint8_t imm = 0;
        if (!byteAt(1, imm)) return fail("Truncated 8051 ADD A,#data");
        mcu.registers().setAccumulator(static_cast<uint8_t>(mcu.registers().accumulator() + imm));
        mcu.pc().advance(2);
        return true;
    }
    if (op >= 0x28 && op <= 0x2F) { // ADD A,Rn
        int reg = 1 + (op - 0x28);
        mcu.registers().setAccumulator(static_cast<uint8_t>(mcu.registers().accumulator() + mcu.registers().get(reg)));
        mcu.pc().advance(1);
        return true;
    }
    if (op == 0x02) { // LJMP addr16
        uint8_t hi = 0, lo = 0;
        if (!byteAt(1, hi) || !byteAt(2, lo)) return fail("Truncated 8051 LJMP");
        mcu.pc().jump(static_cast<uint16_t>((hi << 8) | lo));
        return true;
    }
    if (op == 0xD2 || op == 0xC2) { // SETB/CLR bit, simplified port map
        uint8_t bitAddress = 0;
        if (!byteAt(1, bitAddress)) return fail("Truncated 8051 bit instruction");
        int port = bitAddress / 8;
        int bit = bitAddress % 8;
        if (port > 1) return fail("8051 bit address outside simulated ports");
        if (op == 0xD2) mcu.port(port).setBit(bit);
        else mcu.port(port).clearBit(bit);
        mcu.pc().advance(2);
        return true;
    }
    if (op == 0xE4) { // CLR A
        mcu.registers().setAccumulator(0);
        mcu.pc().advance(1);
        return true;
    }
    if (op == 0x75) { // MOV direct,#data
        uint8_t direct = 0, imm = 0;
        if (!byteAt(1, direct) || !byteAt(2, imm)) return fail("Truncated 8051 MOV direct,#data");
        if (direct == 0x80) mcu.port(Microcontroller::PORT_A).write(imm);
        else if (direct == 0x90) mcu.port(Microcontroller::PORT_B).write(imm);
        else if (direct < mcu.ram().size()) mcu.ram().write(direct, imm);
        else return fail("8051 direct address outside simulated memory");
        mcu.pc().advance(3);
        return true;
    }
    if (op == 0xE5) { // MOV A,direct
        uint8_t direct = 0;
        if (!byteAt(1, direct)) return fail("Truncated 8051 MOV A,direct");
        uint8_t value = 0;
        if (direct == 0x80) value = mcu.port(Microcontroller::PORT_A).read();
        else if (direct == 0x90) value = mcu.port(Microcontroller::PORT_B).read();
        else if (direct < mcu.ram().size()) value = mcu.ram().read(direct);
        else return fail("8051 direct address outside simulated memory");
        mcu.registers().setAccumulator(value);
        mcu.pc().advance(2);
        return true;
    }
    if (op == 0xF5) { // MOV direct,A
        uint8_t direct = 0;
        if (!byteAt(1, direct)) return fail("Truncated 8051 MOV direct,A");
        uint8_t value = mcu.registers().accumulator();
        if (direct == 0x80) mcu.port(Microcontroller::PORT_A).write(value);
        else if (direct == 0x90) mcu.port(Microcontroller::PORT_B).write(value);
        else if (direct < mcu.ram().size()) mcu.ram().write(direct, value);
        else return fail("8051 direct address outside simulated memory");
        mcu.pc().advance(2);
        return true;
    }

    std::ostringstream msg;
    msg << "Unknown MCU opcode 0x" << std::hex << static_cast<int>(op)
        << " at address 0x" << pc;
    return fail(msg.str());
}
