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
#include "../src/measurement/voltage_probe.h"
#include "../src/measurement/voltmeter.h"
#include "../src/measurement/ammeter.h"
#include "../src/measurement/oscilloscope.h"
#include "../src/components/sources/battery.h"
#include "../src/components/sources/clock_gen.h"
#include "../src/components/interactive/switch_comp.h"
#include "../src/components/interactive/push_button.h"
#include "../src/components/interactive/led.h"
#include "../src/components/interactive/seven_segment.h"
#include "../src/simulation/analog_solver.h"


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

static void solveCircuit(Circuit& circuit, double dt = 0.001,
                         double simTime = 0.001) {
    for (int pass = 0; pass < 4; ++pass) {
        for (Component* component : circuit.components) {
            if (!AnalogSolver::ownsComponentStep(*component))
                component->step(pass == 0 ? dt : 0.0, simTime);
        }
        circuit.propagateVoltages();
        AnalogSolver::solve(circuit, pass == 0 ? dt : 0.0,
                            pass == 0 && dt > 0.0);
        circuit.propagateVoltages();
    }
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

static void testRemainingDigitalComponents() {
    XorGate xorGate("X1", 0, 0, 2);
    xorGate.propagationDelay = 0.0;
    xorGate.pins[0].connected = true;
    xorGate.pins[1].connected = true;
    xorGate.pins[0].voltage = 5.0;
    xorGate.pins[1].voltage = 0.0;
    xorGate.step(0.0, 0.0);
    check(near(xorGate.pins[2].voltage, 5.0),
          "XOR outputs HIGH for different inputs");

    NandGate nandGate("Q1", 0, 0, 2);
    nandGate.propagationDelay = 0.0;
    nandGate.pins[0].connected = true;
    nandGate.pins[1].connected = true;
    nandGate.pins[0].voltage = 5.0;
    nandGate.pins[1].voltage = 5.0;
    nandGate.step(0.0, 0.0);
    check(near(nandGate.pins[2].voltage, 0.0),
          "NAND outputs LOW only when every input is HIGH");

    PushButton button("PB1", 0, 0);
    button.press();
    button.step(0.0, 0.0);
    const bool pressedHigh = near(button.pins[0].voltage, 5.0);
    button.release();
    button.step(0.0, 0.0);
    check(pressedHigh && near(button.pins[0].voltage, 0.0),
          "push button drives HIGH while pressed and LOW when released");

    SevenSegment display("SEG1", 0, 0);
    display.pins[0].voltage = 5.0;
    display.pins[1].voltage = 0.0;
    display.step(0.0, 0.0);
    check(display.segOn[0] && !display.segOn[1],
          "seven-segment display follows each segment input independently");
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
    check(source.pins[0].needsInputConnection() &&
          source.pins[1].needsInputConnection(),
          "two-terminal source requires both terminals to be wired");

    Ground legacyOutput("G1", 0, 0);
    check(legacyOutput.pins[0].drivesNet(),
          "legacy isOutput pins still drive nets");

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
    auto* high = new PushButton("PB1", 40, 0);
    high->press();
    high->step(0.0, 0.0);
    shorted.addComponent(high);
    shorted.addWire(&shorted.components[0]->pins[0], &shorted.components[1]->pins[0]);
    shorted.components[0]->pins[0].voltage = 0;
    shorted.components[1]->pins[0].voltage = 5;
    DRC d2;
    check(!d2.checkShorts(shorted), "short circuit detected");

    Circuit unreferenced;
    unreferenced.addComponent(new Ground("G1", 0, 80));
    unreferenced.addComponent(new DCSource("V1", 0, 0, 5));
    unreferenced.addComponent(new Resistor("R1", 60, 0, 1000));
    unreferenced.addWire(&unreferenced.components[1]->pins[0],
                         &unreferenced.components[2]->pins[0]);
    unreferenced.addWire(&unreferenced.components[1]->pins[1],
                         &unreferenced.components[2]->pins[1]);
    DRC d3;
    check(!d3.checkReferences(unreferenced),
          "electrical island disconnected from ground is detected");

    Circuit sourceShort;
    sourceShort.addComponent(new Ground("G1", 0, 0));
    sourceShort.addComponent(new DCSource("V1", 40, 0, 5));
    sourceShort.addWire(&sourceShort.components[0]->pins[0],
                        &sourceShort.components[1]->pins[0]);
    sourceShort.addWire(&sourceShort.components[0]->pins[0],
                        &sourceShort.components[1]->pins[1]);
    DRC d4;
    check(!d4.checkShorts(sourceShort),
          "shorted two-terminal voltage source is detected");
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

static void testGateUndefined() {
    OrGate or1("O1", 0, 0, 2);
    or1.propagationDelay = 0;
    or1.pins[0].connected = true;
    or1.pins[1].connected = true;
    or1.pins[0].voltage = 5;
    or1.pins[1].voltage = 1.4;
    or1.step(0.01, 0);
    check(near(or1.pins[2].voltage, 5), "OR with HIGH input stays HIGH despite undefined");

    or1.pins[0].voltage = 0;
    or1.step(0.01, 0.01);
    check(near(or1.pins[2].voltage, 1.4), "OR with LOW and undefined is undefined");

    OrGate or2("O2", 0, 0, 2);
    or2.propagationDelay = 0;
    or2.pins[0].connected = true;
    or2.pins[1].connected = true;
    or2.pins[0].voltage = 0;
    or2.pins[1].voltage = 5;
    or2.step(0.01, 0);
    check(near(or2.pins[2].voltage, 5), "OR outputs HIGH when one input HIGH");

    or2.pins[0].voltage = 0;
    or2.pins[1].voltage = 0;
    or2.step(0.01, 0.01);
    check(near(or2.pins[2].voltage, 0), "OR outputs LOW when both LOW");

    AndGate and1("A1", 0, 0, 2);
    and1.propagationDelay = 0;
    and1.pins[0].connected = true;
    and1.pins[1].connected = true;
    and1.pins[0].voltage = 0;
    and1.pins[1].voltage = 1.4;
    and1.step(0.01, 0);
    check(near(and1.pins[2].voltage, 0), "AND with LOW input stays LOW despite undefined");
}

static void testVoltageProbe() {
    Circuit c;
    Ground* g = new Ground("G1", 0, 0);
    DCSource* v = new DCSource("V1", 40, 0, 5);
    Resistor* r = new Resistor("R1", 80, 0, 100);
    c.addComponent(g);
    c.addComponent(v);
    c.addComponent(r);
    c.addWire(&g->pins[0], &v->pins[0]);
    c.addWire(&v->pins[1], &r->pins[0]);
    c.addWire(&r->pins[1], &g->pins[0]);
    solveCircuit(c);

    VoltageProbe probe;
    check(near(probe.read(c, v->pins[1].worldPos()), 5), "probe reads source HIGH voltage");
    check(near(probe.read(c, g->pins[0].worldPos()), 0), "probe reads ground voltage");
    check(!probe.isFloating(c, v->pins[1].worldPos()), "probe detects connected net");
    check(probe.isFloating(c, Vector2D(1000, 1000)), "probe detects floating at empty position");
}

static void testVoltmeter() {
    Circuit noGround;
    Voltmeter* vm1 = new Voltmeter("VM1", 0, 0);
    DCSource* s1 = new DCSource("V1", 40, 0, 5);
    noGround.addComponent(vm1);
    noGround.addComponent(s1);
    noGround.addWire(&vm1->pins[0], &s1->pins[0]);
    vm1->step(0.01, 0);
    check(vm1->hasError, "voltmeter errors without ground reference");

    Circuit c;
    Ground* g = new Ground("G1", 0, 0);
    DCSource* v = new DCSource("V1", 40, 0, 5);
    Voltmeter* vm2 = new Voltmeter("VM2", 80, 0);
    c.addComponent(g);
    c.addComponent(v);
    c.addComponent(vm2);
    c.addWire(&v->pins[0], &g->pins[0]);
    c.addWire(&vm2->pins[0], &v->pins[1]);
    c.addWire(&vm2->pins[1], &g->pins[0]);
    solveCircuit(c);
    check(!vm2->hasError, "voltmeter ok with ground and connected pins");
    check(near(vm2->reading, 5), "voltmeter reads correct voltage difference");
}

static void testAmmeter() {
    Circuit c;
    Ground* g = new Ground("G1", 0, 0);
    DCSource* v = new DCSource("V1", 40, 0, 5);
    Ammeter* am = new Ammeter("AM1", 80, 0);
    Resistor* r = new Resistor("R1", 120, 0, 100);
    c.addComponent(g);
    c.addComponent(v);
    c.addComponent(am);
    c.addComponent(r);

    c.addWire(&v->pins[0], &g->pins[0]);
    c.addWire(&v->pins[1], &am->pins[0]);
    c.addWire(&am->pins[1], &r->pins[0]);
    c.addWire(&r->pins[1], &g->pins[0]);

    solveCircuit(c);

    check(am->reading > 0, "ammeter reads positive current through series resistor");
}

static void testOscilloscope() {
    Circuit c;
    Ground* g = new Ground("G1", 0, 0);
    DCSource* v = new DCSource("V1", 40, 0, 5);
    c.addComponent(g);
    c.addComponent(v);
    Resistor* r = new Resistor("R1", 80, 0, 1000);
    c.addComponent(r);
    c.addWire(&g->pins[0], &v->pins[0]);
    c.addWire(&v->pins[1], &r->pins[0]);
    c.addWire(&r->pins[1], &g->pins[0]);
    solveCircuit(c);

    int highNetId = -1;
    for (Net* n : c.nets) {
        if (n->hasPin(&v->pins[1]))
            highNetId = n->id;
    }

    Oscilloscope scope;
    if (scope.channels.empty())
        scope.channels.resize(2);
    scope.attachChannel(0, highNetId);
    check(scope.channels[0].netId == highNetId, "oscilloscope channel attached to net");

    scope.start();
    check(scope.isRunning, "oscilloscope starts running");

    scope.update(c, 0.0);
    scope.update(c, 0.01);
    scope.update(c, 0.02);
    check(scope.channels[0].history.size() == 3, "oscilloscope records samples while running");

    scope.pause();
    scope.update(c, 0.03);
    check(scope.channels[0].history.size() == 3, "oscilloscope stops recording when paused");

    scope.stop();
    check(scope.channels[0].history.empty(), "oscilloscope clears history on stop");
}

static void testBattery() {
    Circuit c;
    Battery* bat = new Battery("BAT1", 0, 0, 9);
    bat->internalResistance = 1.0;
    c.addComponent(bat);

    bat->pins[0].voltage = 0;
    bat->pins[0].connected = true;
    bat->step(0.01, 0);
    check(near(bat->pins[1].voltage, 9), "battery outputs rated voltage without load");
}

static void testClockGenerator() {
    ClockGenerator clk("CLK1", 0, 0, 1.0);
    clk.highVoltage = 5.0;

    clk.step(0.01, 0.0);
    check(near(clk.pins[0].voltage, 5), "clock outputs HIGH at start of period");

    clk.step(0.01, 0.6);
    check(near(clk.pins[0].voltage, 0), "clock outputs LOW in second half of period");

    clk.step(0.01, 1.1);
    check(near(clk.pins[0].voltage, 5), "clock outputs HIGH at start of next period");
}

static void testSwitch() {
    Circuit c;
    Switch* sw = new Switch("SW1", 0, 0);
    DCSource* v = new DCSource("V1", 40, 0, 5);
    Resistor* r = new Resistor("R1", 80, 0, 100);
    Ground* g = new Ground("G1", 0, 80);
    c.addComponent(sw);
    c.addComponent(v);
    c.addComponent(r);
    c.addComponent(g);

    c.addWire(&v->pins[1], &sw->pins[0]);
    c.addWire(&sw->pins[1], &r->pins[0]);
    c.addWire(&r->pins[1], &g->pins[0]);
    c.addWire(&v->pins[0], &g->pins[0]);
    solveCircuit(c);

    check(!sw->pins[1].drivesNet(), "open switch isolates output from input");

    sw->toggle();
    check(sw->closed, "toggle changes switch state to closed");

    solveCircuit(c, 0.001, 0.01);

    check(near(sw->pins[1].voltage, 5), "closed switch propagates voltage to output");
}

static void testLED() {
    Circuit c;
    LED* led = new LED("LED1", 0, 0, "red");
    led->threshold = 2.0;
    c.addComponent(led);

    led->pins[0].voltage = 1.0;
    led->pins[1].voltage = 0.0;
    led->step(0.01, 0);
    check(!led->lit, "LED off when voltage below threshold");

    led->pins[0].voltage = 3.0;
    led->step(0.01, 0.01);
    check(led->lit, "LED on when voltage above threshold");
}




int main() {
    testWireRouting();
    testMoveComponent();
    testUIHelpers();
    testSerialize();
    testUndoRedo();
    testGates();
    testRemainingDigitalComponents();
    testGateUndefined();
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
    testVoltageProbe();
    testVoltmeter();
    testAmmeter();
    testOscilloscope();
    testBattery();
    testClockGenerator();
    testSwitch();
    testLED();
    cout << "\n" << passed << " passed, " << failed << " failed" << endl;
    return failed == 0 ? 0 : 1;
}
