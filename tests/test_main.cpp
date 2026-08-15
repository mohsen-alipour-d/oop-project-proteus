#include <iostream>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../src/core/circuit.h"
#include "../src/components/sources/ground.h"
#include "../src/components/sources/dc_source.h"
#include "../src/components/passive/resistor.h"
#include "../src/components/passive/capacitor.h"
#include "../src/components/digital/logic_gates.h"
#include "../src/components/digital/d_flipflop.h"
#include "../src/components/advanced/adc.h"
#include "../src/components/advanced/dac.h"
#include "../src/components/advanced/microcontroller.h"
#include "../src/components/advanced/external_memory.h"
#include "../src/components/advanced/lcd16x2.h"
#include "../src/components/advanced/matrix_keypad.h"
#include "../src/mcu/firmware_loader.h"
#include "../src/file/file_manager.h"
#include "../src/file/history.h"
#include "../src/drc/drc.h"

using namespace std;

static int passed = 0;
static int failed = 0;


static void check(bool cond, const string& name) {
    if (cond) {
        passed++;
        cout << "PASS " << name << endl;
    } else {
        failed++;
        cout << "FAIL " << name << endl;
    }
}

static bool near(double a, double b) {
    return fabs(a - b) < 0.0001;
}

static void testWireRouting() {
    Circuit c;
    c.addComponent(new Resistor("R1", 0, 0, 100));
    c.addComponent(new Resistor("R2", 40, 30, 100));
    Wire* w = c.addWire(&c.components[0]->pins[0], &c.components[1]->pins[0]);
    check(w->points.size() == 3, "wire has 3 points for 90 degree route");
    check(near(w->points[1].x, w->points[2].x), "middle point shares x with end");
}

static void testMoveComponent() {
    Circuit c;
    c.addComponent(new DCSource("V1", 0, 0, 5));
    c.addComponent(new Resistor("R1", 60, 0, 100));
    Wire* w = c.addWire(&c.components[0]->pins[0], &c.components[1]->pins[0]);
    double oldEndX = w->points.back().x;
    c.moveComponent(c.components[1], 20, 0);
    check(near(w->points.back().x, oldEndX + 20), "wire endpoint follows moved component");
}

static void testUIHelpers() {
    Circuit c;
    Component* r = new Resistor("R1", 53, 47, 100);
    c.addComponent(r);
    r->snapToGrid(10);
    check(near(r->position.x, 50) && near(r->position.y, 50), "snap to grid rounds position");
    Rect box = r->getBoundingBox();
    check(box.contains(Vector2D(50, 50)), "bounding box contains component center");
    check(c.getComponentsInRect(box).size() == 1, "rect selection finds component");
}

static void testSerialize() {
    Circuit c;
    c.addComponent(new Ground("G1", 0, 0));
    c.addComponent(new DCSource("V1", 40, 0, 5));
    c.addComponent(new Resistor("R1", 80, 0, 220));
    c.addWire(&c.components[1]->pins[0], &c.components[2]->pins[0]);
    FileManager fm;
    Circuit loaded;
    fm.deserialize(loaded, fm.serialize(c));
    check(loaded.components.size() == 3, "deserialize restores component count");
    check(loaded.wires.size() == 1, "deserialize restores wire count");
    check(near(((Resistor*)loaded.components[2])->value, 220), "deserialize restores resistor value");
}

static void testUndoRedo() {
    Circuit c;
    c.addComponent(new Resistor("R1", 0, 0, 100));
    FileManager fm;
    History h;
    h.push(fm.serialize(c));
    ((Resistor*)c.components[0])->value = 330;
    h.push(fm.serialize(c));
    fm.deserialize(c, h.undo());
    check(near(((Resistor*)c.components[0])->value, 100), "undo restores old value");
    fm.deserialize(c, h.redo());
    check(near(((Resistor*)c.components[0])->value, 330), "redo restores new value");
}

static void testGates() {
    NotGate not1("N1", 0, 0);
    not1.propagationDelay = 0;
    not1.pins[0].connected = true;
    not1.pins[0].voltage = 5;
    not1.step(0.01, 0);
    check(near(not1.pins[1].voltage, 0), "NOT inverts HIGH to LOW");

    AndGate and1("A1", 0, 0, 2);
    and1.propagationDelay = 0;
    and1.pins[0].connected = true;
    and1.pins[1].connected = true;
    and1.pins[0].voltage = 5;
    and1.pins[1].voltage = 5;
    and1.step(0.01, 0);
    check(near(and1.pins[2].voltage, 5), "AND outputs HIGH when both HIGH");
}

static void testFlipFlop() {
    DFlipFlop ff("F1", 0, 0);
    ff.pins[0].connected = true;
    ff.pins[1].connected = true;
    ff.pins[0].voltage = 5;
    ff.pins[1].voltage = 0;
    ff.step(0.01, 0);
    ff.pins[1].voltage = 5;
    ff.step(0.01, 0.01);
    check(near(ff.pins[2].voltage, 5), "D-FF captures D on rising edge");
}


static void testPinDirectionCompatibility() {
    Resistor inputLike("R1", 0, 0, 100);
    check(inputLike.pins[0].needsInputConnection(), "default pin remains an input");

    DCSource source("V1", 0, 0, 5);
    check(source.pins[0].drivesNet(), "legacy isOutput pins still drive nets");

    Pin bidirectional(0, 0, &inputLike);
    bidirectional.setDirection(PinDirection::Bidirectional);
    check(bidirectional.needsInputConnection(), "bidirectional pin is input while not driving");
    bidirectional.setDriving(true);
    check(bidirectional.drivesNet() && !bidirectional.needsInputConnection(), "bidirectional pin can actively drive");
}

static void testADC() {
    ADC adc("ADC1", 0, 0, 8, 0.0);
    adc.pins[ADC::VREF_MINUS_PIN].voltage = 0.0;
    adc.pins[ADC::VREF_PLUS_PIN].voltage = 5.0;

    adc.pins[ADC::VIN_PIN].voltage = 0.0;
    adc.step(0.0, 0.0);
    check(adc.getOutputCode() == 0, "ADC maps Vref- to zero");

    adc.pins[ADC::VIN_PIN].voltage = 5.0;
    adc.step(0.0, 0.0);
    check(adc.getOutputCode() == 255, "ADC maps Vref+ to full scale");

    adc.pins[ADC::VIN_PIN].voltage = -2.0;
    adc.step(0.0, 0.0);
    check(adc.getOutputCode() == 0, "ADC saturates below reference range");

    adc.pins[ADC::VIN_PIN].voltage = 9.0;
    adc.step(0.0, 0.0);
    check(adc.getOutputCode() == 255, "ADC saturates above reference range");

    ADC adc4("ADC4", 0, 0, 4, 0.0);
    adc4.pins[ADC::VREF_MINUS_PIN].voltage = 0.0;
    adc4.pins[ADC::VREF_PLUS_PIN].voltage = 5.0;
    adc4.pins[ADC::VIN_PIN].voltage = 5.0;
    adc4.step(0.0, 0.0);
    check(adc4.getOutputCode() == 15 && adc4.pins.size() == 7, "ADC supports configurable bit width");
}

static void testADCDelay() {
    ADC adc("ADC1", 0, 0, 8, 1.0);
    adc.pins[ADC::VREF_MINUS_PIN].voltage = 0.0;
    adc.pins[ADC::VREF_PLUS_PIN].voltage = 5.0;
    adc.pins[ADC::VIN_PIN].voltage = 5.0;

    adc.step(0.0, 0.0);
    check(adc.getOutputCode() == 0, "ADC holds previous output during conversion delay");
    adc.step(0.0, 0.5);
    check(adc.getOutputCode() == 0, "ADC output stays unchanged before delay expires");
    adc.step(0.0, 1.0);
    check(adc.getOutputCode() == 255, "ADC applies converted value after delay");
}

static void testADCSerialize() {
    Circuit c;
    c.addComponent(new ADC("ADC1", 10, 20, 10, 0.25));
    FileManager fm;
    Circuit loaded;
    fm.deserialize(loaded, fm.serialize(c));

    check(loaded.components.size() == 1 && loaded.components[0]->typeTag() == "ADC", "ADC survives save/load factory reconstruction");
    ADC* adc = dynamic_cast<ADC*>(loaded.components[0]);
    check(adc != nullptr && adc->getBitCount() == 10 && near(adc->getConversionDelay(), 0.25), "ADC save/load preserves configuration");
}


static void testDAC() {
    DAC dac("DAC1", 0, 0, 8, 0.0);
    dac.pins[dac.vrefMinusPinIndex()].voltage = 0.0;
    dac.pins[dac.vrefPlusPinIndex()].voltage = 5.0;
    for (int bit = 0; bit < 8; ++bit)
        dac.pins[dac.dataPinIndex(bit)].voltage = 0.0;
    dac.step(0.0, 0.0);
    check(near(dac.getOutputVoltage(), 0.0), "DAC maps zero code to Vref-");

    for (int bit = 0; bit < 8; ++bit)
        dac.pins[dac.dataPinIndex(bit)].voltage = 5.0;
    dac.step(0.0, 0.0);
    check(near(dac.getOutputVoltage(), 5.0), "DAC maps full-scale code to Vref+");

    DAC dac4("DAC4", 0, 0, 4, 0.0);
    dac4.pins[dac4.vrefMinusPinIndex()].voltage = -1.0;
    dac4.pins[dac4.vrefPlusPinIndex()].voltage = 1.0;
    for (int bit = 0; bit < 4; ++bit)
        dac4.pins[dac4.dataPinIndex(bit)].voltage = ((7 >> bit) & 1) ? 5.0 : 0.0;
    dac4.step(0.0, 0.0);
    check(near(dac4.getOutputVoltage(), -1.0 + (7.0 / 15.0) * 2.0), "DAC linearly maps configurable input width");
}

static void testDACDelay() {
    DAC dac("DAC1", 0, 0, 8, 1.0);
    dac.pins[dac.vrefMinusPinIndex()].voltage = 0.0;
    dac.pins[dac.vrefPlusPinIndex()].voltage = 5.0;
    for (int bit = 0; bit < 8; ++bit)
        dac.pins[dac.dataPinIndex(bit)].voltage = 5.0;

    dac.step(0.0, 0.0);
    check(near(dac.getOutputVoltage(), 0.0), "DAC holds old output during conversion delay");
    dac.step(0.0, 0.5);
    check(near(dac.getOutputVoltage(), 0.0), "DAC output remains stable before delay expires");
    dac.step(0.0, 1.0);
    check(near(dac.getOutputVoltage(), 5.0), "DAC updates after conversion delay");
}

static string intelHexRecord(uint16_t address, uint8_t type, const vector<uint8_t>& data) {
    uint8_t count = static_cast<uint8_t>(data.size());
    uint8_t sum = count + static_cast<uint8_t>(address >> 8) + static_cast<uint8_t>(address & 0xFF) + type;
    for (uint8_t b : data)
        sum = static_cast<uint8_t>(sum + b);
    uint8_t checksum = static_cast<uint8_t>(0 - sum);

    stringstream out;
    out << ':' << uppercase << hex << setfill('0')
        << setw(2) << static_cast<int>(count)
        << setw(4) << static_cast<int>(address)
        << setw(2) << static_cast<int>(type);
    for (uint8_t b : data)
        out << setw(2) << static_cast<int>(b);
    out << setw(2) << static_cast<int>(checksum);
    return out.str();
}

static void testFirmwareLoader() {
    const string validPath = "part7_valid_firmware.hex";
    const string badPath = "part7_bad_firmware.hex";
    const vector<uint8_t> program = {0x10, 0x00, 0x05, 0x20, 0x00, 0x03};

    {
        ofstream f(validPath);
        f << intelHexRecord(0, 0x00, program) << "\n";
        f << intelHexRecord(0, 0x01, {}) << "\n";
    }

    FlashMemory flash;
    FirmwareLoader loader;
    check(loader.loadHex(validPath, flash), "firmware loader accepts valid Intel HEX");
    check(flash.read(0) == 0x10 && flash.read(5) == 0x03, "firmware loader places opcodes into Flash");

    {
        string bad = intelHexRecord(0, 0x00, program);
        bad.back() = bad.back() == '0' ? '1' : '0';
        ofstream f(badPath);
        f << bad << "\n";
        f << intelHexRecord(0, 0x01, {}) << "\n";
    }
    FlashMemory badFlash;
    check(!loader.loadHex(badPath, badFlash), "firmware loader rejects bad Intel HEX checksum");

    remove(validPath.c_str());
    remove(badPath.c_str());
}

static void testMCUStateAndDecoder() {
    Microcontroller mcu("MCU1", 0, 0, 0.001);

    // MOV A,#5 ; ADD A,#3 ; SETB A.2 ; CLR A.2 ; JMP 0x0003
    vector<uint8_t> program = {
        0x10, 0x00, 0x05,
        0x20, 0x00, 0x03,
        0x40, 0x00, 0x02,
        0x50, 0x00, 0x02,
        0x30, 0x00, 0x03
    };
    for (size_t i = 0; i < program.size(); ++i)
        mcu.flash().write(i, program[i]);

    check(mcu.executeOneInstruction() && mcu.registers().accumulator() == 5 && mcu.pc().value() == 3,
          "MCU MOV updates register and PC");
    check(mcu.executeOneInstruction() && mcu.registers().accumulator() == 8 && mcu.pc().value() == 6,
          "MCU ADD updates register");
    check(mcu.executeOneInstruction() && (mcu.port(Microcontroller::PORT_A).latch() & 0x04),
          "MCU SETB sets output port bit");
    check(mcu.executeOneInstruction() && !(mcu.port(Microcontroller::PORT_A).latch() & 0x04),
          "MCU CLR clears output port bit");
    check(mcu.executeOneInstruction() && mcu.pc().value() == 3, "MCU JMP changes program counter");

    mcu.ram().write(12, 0xAB);
    check(mcu.ram().read(12) == 0xAB, "MCU internal RAM read/write works");
    mcu.registers().set(1, 0x44);
    check(mcu.registers().get(1) == 0x44, "MCU general register file works");
}

static void testMCUPorts() {
    Microcontroller mcu("MCU1", 0, 0);
    mcu.port(Microcontroller::PORT_A).write(0xA5);
    check(mcu.port(Microcontroller::PORT_A).read() == 0xA5, "MCU output port preserves written latch value");

    mcu.port(Microcontroller::PORT_A).setDirectionMask(0x00);
    for (int bit = 0; bit < 8; ++bit)
        mcu.pins[mcu.portPinIndex(Microcontroller::PORT_A, bit)].voltage = ((0x3C >> bit) & 1) ? 5.0 : 0.0;
    check(mcu.port(Microcontroller::PORT_A).read() == 0x3C, "MCU input port samples external pin voltages");
}

static void test8051Subset() {
    Microcontroller mcu("MCU1", 0, 0);
    // MOV A,#10 ; ADD A,#5 ; MOV P0,A ; CLR A ; LJMP 0000
    vector<uint8_t> program = {0x74, 10, 0x24, 5, 0xF5, 0x80, 0xE4, 0x02, 0x00, 0x00};
    for (size_t i = 0; i < program.size(); ++i)
        mcu.flash().write(i, program[i]);
    mcu.executeOneInstruction();
    mcu.executeOneInstruction();
    check(mcu.registers().accumulator() == 15, "MCU supports 8051-style MOV/ADD subset");
    mcu.executeOneInstruction();
    check(mcu.port(Microcontroller::PORT_A).latch() == 15, "8051-style MOV direct,A can write MCU port");
    mcu.executeOneInstruction();
    check(mcu.registers().accumulator() == 0, "8051-style CLR A works");
    mcu.executeOneInstruction();
    check(mcu.pc().value() == 0, "8051-style LJMP works");
}

static void testExternalMemory() {
    ExternalMemory mem("RAM1", 0, 0, 4);
    // Address 3.
    for (int bit = 0; bit < 4; ++bit)
        mem.pins[mem.addressPinIndex(bit)].voltage = ((3 >> bit) & 1) ? 5.0 : 0.0;
    // Data 0xA6.
    for (int bit = 0; bit < 8; ++bit)
        mem.pins[mem.dataPinIndex(bit)].voltage = ((0xA6 >> bit) & 1) ? 5.0 : 0.0;
    mem.pins[mem.readPinIndex()].voltage = 0.0;
    mem.pins[mem.writePinIndex()].voltage = 5.0;
    mem.step(0.0, 0.0);
    check(mem.readByte(3) == 0xA6, "external memory writes data through address/data buses");

    mem.pins[mem.writePinIndex()].voltage = 0.0;
    mem.pins[mem.readPinIndex()].voltage = 5.0;
    mem.step(0.0, 0.1);
    uint8_t read = 0;
    for (int bit = 0; bit < 8; ++bit)
        if (mem.pins[mem.dataPinIndex(bit)].drivesNet() && mem.pins[mem.dataPinIndex(bit)].voltage >= 2.0)
            read |= static_cast<uint8_t>(1u << bit);
    check(read == 0xA6, "external memory drives stored data during read cycle");
}

static void setLCDBus(LCD16x2& lcd, uint8_t byte) {
    for (int bit = 0; bit < 8; ++bit)
        lcd.pins[lcd.dataPinIndex(bit)].voltage = ((byte >> bit) & 1u) ? 5.0 : 0.0;
}

static void pulseLCD(LCD16x2& lcd, bool dataMode, uint8_t byte, double t) {
    setLCDBus(lcd, byte);
    lcd.pins[LCD16x2::RS_PIN].voltage = dataMode ? 5.0 : 0.0;
    lcd.pins[LCD16x2::RW_PIN].voltage = 0.0;
    lcd.pins[LCD16x2::ENABLE_PIN].voltage = 0.0;
    lcd.step(0.0, t);
    lcd.pins[LCD16x2::ENABLE_PIN].voltage = 5.0;
    lcd.step(0.0, t + 0.001);
    lcd.pins[LCD16x2::ENABLE_PIN].voltage = 0.0;
    lcd.step(0.0, t + 0.002);
}

static void testLCD() {
    LCD16x2 lcd("LCD1", 0, 0);
    pulseLCD(lcd, true, 'H', 0.0);
    pulseLCD(lcd, true, 'i', 0.01);
    check(lcd.line1().substr(0, 2) == "Hi", "LCD displays received characters");

    pulseLCD(lcd, false, 0xC0, 0.02); // row 2, column 0
    pulseLCD(lcd, true, 'X', 0.03);
    check(lcd.line2()[0] == 'X', "LCD cursor command selects second row");

    pulseLCD(lcd, false, 0x01, 0.04);
    check(lcd.line1() == string(16, ' ') && lcd.line2() == string(16, ' '), "LCD clear command clears 16x2 display");
}

static void testKeypad() {
    MatrixKeypad keypad("KPD1", 0, 0);
    keypad.press(1, 2);
    keypad.pins[keypad.colPinIndex(2)].voltage = 5.0;
    keypad.step(0.0, 0.0);
    check(keypad.pins[keypad.rowPinIndex(1)].drivesNet() && near(keypad.pins[keypad.rowPinIndex(1)].voltage, 5.0),
          "keypad scan propagates active column to pressed row");
    check(keypad.keyLabel(1, 2) == '6', "keypad exposes standard 4x4 key mapping");
    keypad.release(1, 2);
    keypad.step(0.0, 0.1);
    check(!keypad.pins[keypad.rowPinIndex(1)].drivesNet(), "keypad row stops driving after key release");
}

static void testAdvancedSaveLoad() {
    Circuit c;
    c.addComponent(new DAC("DAC1", 0, 0, 6, 0.2));
    Microcontroller* mcu = new Microcontroller("MCU1", 40, 0, 0.005);
    mcu->flash().write(0, 0x74);
    mcu->flash().write(1, 0x2A);
    mcu->port(Microcontroller::PORT_A).setDirectionMask(0x0F);
    c.addComponent(mcu);
    ExternalMemory* mem = new ExternalMemory("RAM1", 80, 0, 3);
    mem->writeByte(2, 0x5A);
    c.addComponent(mem);
    c.addComponent(new LCD16x2("LCD1", 120, 0));
    c.addComponent(new MatrixKeypad("KP1", 160, 0));

    FileManager fm;
    Circuit loaded;
    fm.deserialize(loaded, fm.serialize(c));
    check(loaded.components.size() == 5, "save/load restores all advanced Part 7 component types");

    DAC* dac = dynamic_cast<DAC*>(loaded.components[0]);
    Microcontroller* lmcu = dynamic_cast<Microcontroller*>(loaded.components[1]);
    ExternalMemory* lmem = dynamic_cast<ExternalMemory*>(loaded.components[2]);
    check(dac && dac->getBitCount() == 6 && near(dac->getConversionDelay(), 0.2), "DAC configuration survives save/load");
    check(lmcu && lmcu->flash().read(0) == 0x74 && lmcu->flash().read(1) == 0x2A &&
          lmcu->port(Microcontroller::PORT_A).directionMask() == 0x0F,
          "MCU firmware and port configuration survive save/load");
    check(lmem && lmem->readByte(2) == 0x5A, "external memory contents survive save/load");
    check(dynamic_cast<LCD16x2*>(loaded.components[3]) != nullptr && dynamic_cast<MatrixKeypad*>(loaded.components[4]) != nullptr,
          "LCD and keypad survive save/load factory reconstruction");
}

static void testMCUToLCDIntegration() {
    Circuit c;
    auto* mcu = new Microcontroller("MCU1", 0, 0);
    auto* lcd = new LCD16x2("LCD1", 100, 0);
    c.addComponent(mcu);
    c.addComponent(lcd);

    for (int bit = 0; bit < 8; ++bit)
        c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_A, bit)], &lcd->pins[lcd->dataPinIndex(bit)]);
    c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_B, 0)], &lcd->pins[LCD16x2::RS_PIN]);
    c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_B, 1)], &lcd->pins[LCD16x2::RW_PIN]);
    c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_B, 2)], &lcd->pins[LCD16x2::ENABLE_PIN]);

    mcu->port(Microcontroller::PORT_A).write(static_cast<uint8_t>('Z'));
    mcu->port(Microcontroller::PORT_B).write(0x01); // RS=1, RW=0, E=0
    c.propagateVoltages();
    lcd->step(0.0, 0.0);
    mcu->port(Microcontroller::PORT_B).write(0x05); // E rising
    c.propagateVoltages();
    lcd->step(0.0, 0.01);
    check(lcd->line1()[0] == 'Z', "MCU GPIO can drive LCD through existing wire/net system");
}

static void testKeypadToMCUIntegration() {
    Circuit c;
    auto* mcu = new Microcontroller("MCU1", 0, 0);
    auto* keypad = new MatrixKeypad("KP1", 100, 0);
    c.addComponent(mcu);
    c.addComponent(keypad);

    mcu->port(Microcontroller::PORT_A).setDirectionMask(0xFE); // A0 input
    for (int col = 0; col < 4; ++col)
        c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_B, col)], &keypad->pins[keypad->colPinIndex(col)]);
    c.addWire(&keypad->pins[keypad->rowPinIndex(0)], &mcu->pins[mcu->portPinIndex(Microcontroller::PORT_A, 0)]);

    keypad->press(0, 2);
    mcu->port(Microcontroller::PORT_B).write(1u << 2);
    c.propagateVoltages();
    keypad->step(0.0, 0.0);
    c.propagateVoltages();
    check((mcu->port(Microcontroller::PORT_A).read() & 0x01) != 0,
          "MCU can detect pressed keypad key through row/column scanning");
}

static void testMCUExternalMemoryIntegration() {
    Circuit c;
    auto* mcu = new Microcontroller("MCU1", 0, 0);
    auto* mem = new ExternalMemory("RAM1", 100, 0, 2);
    c.addComponent(mcu);
    c.addComponent(mem);

    // Port A: A0, A1, RD, WR. Port B: D0..D7.
    for (int bit = 0; bit < 2; ++bit)
        c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_A, bit)], &mem->pins[mem->addressPinIndex(bit)]);
    c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_A, 2)], &mem->pins[mem->readPinIndex()]);
    c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_A, 3)], &mem->pins[mem->writePinIndex()]);
    for (int bit = 0; bit < 8; ++bit)
        c.addWire(&mcu->pins[mcu->portPinIndex(Microcontroller::PORT_B, bit)], &mem->pins[mem->dataPinIndex(bit)]);

    // Write 0xC3 to address 2: A1=1, WR=1 -> 0b1010 on low nibble.
    mcu->port(Microcontroller::PORT_A).write(0x0A);
    mcu->port(Microcontroller::PORT_B).setDirectionMask(0xFF);
    mcu->port(Microcontroller::PORT_B).write(0xC3);
    c.propagateVoltages();
    mem->step(0.0, 0.0);
    check(mem->readByte(2) == 0xC3, "MCU writes external memory through address/data/control buses");

    // Read the same byte back: address 2 + RD, Port B becomes input.
    mcu->port(Microcontroller::PORT_A).write(0x06);
    mcu->port(Microcontroller::PORT_B).setDirectionMask(0x00);
    c.propagateVoltages();
    mem->step(0.0, 0.1);
    c.propagateVoltages();
    check(mcu->port(Microcontroller::PORT_B).read() == 0xC3,
          "MCU reads external memory through bidirectional data bus");
}

static void testDRC() {
    Circuit floating;
    floating.addComponent(new AndGate("A1", 0, 0, 2));
    DRC d1;
    check(!d1.checkFloating(floating), "floating input detected");

    Circuit shorted;
    shorted.addComponent(new Ground("G1", 0, 0));
    shorted.addComponent(new DCSource("V1", 40, 0, 5));
    shorted.addWire(&shorted.components[0]->pins[0], &shorted.components[1]->pins[0]);
    shorted.components[0]->pins[0].voltage = 0;
    shorted.components[1]->pins[0].voltage = 5;
    DRC d2;
    check(!d2.checkShorts(shorted), "short circuit detected");
}

static void testStateSerialization() {
    Circuit c;
    Capacitor* cap = new Capacitor("C1", 0, 0, 0.000001);
    cap->lastVoltage = 3.5;
    DFlipFlop* ff = new DFlipFlop("F1", 40, 0);
    ff->stored = HIGH;
    c.addComponent(cap);
    c.addComponent(ff);
    FileManager fm;
    Circuit loaded;
    fm.deserialize(loaded, fm.serialize(c));
    check(near(((Capacitor*)loaded.components[0])->lastVoltage, 3.5), "capacitor state restored");
    check(((DFlipFlop*)loaded.components[1])->stored == HIGH, "flip-flop state restored");
}


int main() {
    testWireRouting();
    testMoveComponent();
    testUIHelpers();
    testSerialize();
    testUndoRedo();
    testGates();
    testFlipFlop();
    testPinDirectionCompatibility();
    testADC();
    testADCDelay();
    testADCSerialize();
    testDAC();
    testDACDelay();
    testFirmwareLoader();
    testMCUStateAndDecoder();
    testMCUPorts();
    test8051Subset();
    testExternalMemory();
    testLCD();
    testKeypad();
    testAdvancedSaveLoad();
    testMCUToLCDIntegration();
    testKeypadToMCUIntegration();
    testMCUExternalMemoryIntegration();
    testDRC();
    testStateSerialization();
    cout << "\n" << passed << " passed, " << failed << " failed" << endl;
    return failed == 0 ? 0 : 1;
}