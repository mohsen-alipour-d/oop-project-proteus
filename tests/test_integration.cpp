#include <cstdio>
#include <cmath>
#include <iostream>
#include <vector>

#include "integration/backend_adapter.h"

namespace
{
int failures = 0;

void check(bool condition, const std::string& name)
{
    if (condition)
    {
        std::cout << "[PASS] " << name << '\n';
    }
    else
    {
        std::cout << "[FAIL] " << name << '\n';
        ++failures;
    }
}
}


int main()
{
    BackendAdapter backend;

    check(PinHandle{1, 0} != PinHandle{1, 1} &&
          !(PinHandle{1, 0} != PinHandle{1, 0}),
          "pin handles support C++17 inequality comparison");

    const int ground = backend.addComponent(7, "GND1", "0V", {0.0, 80.0});
    const int source = backend.addComponent(2, "BAT1", "5V", {0.0, 0.0});
    const int resistor = backend.addComponent(0, "R1", "1K", {100.0, 0.0});

    check(ground >= 0 && source >= 0 && resistor >= 0,
          "frontend definitions create backend components");
    check(backend.addComponent(0, "R1", "2K", {200.0, 0.0}) < 0,
          "duplicate labels are rejected");
    check(backend.connectPins({source, 0}, {ground, 0}),
          "battery negative terminal is referenced to ground");
    check(backend.connectPins({source, 1}, {resistor, 0}),
          "battery positive terminal feeds the load");
    check(backend.connectPins({ground, 0}, {resistor, 1}),
          "load return is connected");
    check(backend.wireViews().size() == 3,
          "backend wires are visible to the frontend");
    check(backend.findPinAt({41.0, 0.0}, 1.0).has_value(),
          "pin hit testing supports automatic pin selection");

    const int measuredWireId = backend.wireViews().front().id;
    check(backend.attachOscilloscopeChannel(0, measuredWireId),
          "a frontend-selected wire attaches to an oscilloscope channel");
    check(backend.simulationState() == SimulationState::Stopped &&
          backend.simulationTimeSeconds() == 0.0,
          "simulation starts in stopped edit mode");
    backend.startSimulation();
    backend.updateSimulation(0.01);
    const std::vector<ScopeChannelView> scopeChannels =
            backend.oscilloscopeChannels();
    check(scopeChannels.size() == 2 &&
          scopeChannels.front().netId >= 0 &&
          !scopeChannels.front().history.empty(),
          "oscilloscope samples the attached backend net");
    check(backend.simulationState() == SimulationState::Running &&
          std::abs(backend.simulationTimeSeconds() - 0.01) < 1.0e-9,
          "Run advances the internal simulation clock");
    backend.pauseSimulation();
    const double pausedTime = backend.simulationTimeSeconds();
    backend.updateSimulation(0.02);
    check(backend.simulationState() == SimulationState::Paused &&
          backend.simulationTimeSeconds() == pausedTime,
          "Pause freezes time and pending simulation state");
    check(backend.stepSimulation() &&
          backend.simulationState() == SimulationState::Paused &&
          std::abs(backend.simulationTimeSeconds() - pausedTime - 0.001) < 1.0e-9,
          "Step advances exactly 1 ms and remains paused");
    backend.stopSimulation();
    bool stoppedWiresUseDefaultState = true;
    for (const WireView& wire : backend.wireViews())
    {
        stoppedWiresUseDefaultState =
                wire.state == WireLogicState::Floating &&
                stoppedWiresUseDefaultState;
    }
    check(backend.simulationState() == SimulationState::Stopped &&
          backend.simulationTimeSeconds() == 0.0 &&
          stoppedWiresUseDefaultState,
          "Stop resets time, runtime events and live wire state");
    check(backend.validate(), "integrated circuit passes DRC");

    backend.recordHistory();
    std::vector<ComponentInstance> frontend = backend.frontendComponents();
    for (ComponentInstance& component : frontend)
    {
        if (component.id() == resistor)
        {
            component.setPosition({180.0, 40.0});
            check(backend.syncComponent(component),
                  "frontend movement is synchronized to backend");
        }
    }
    backend.recordHistory();
    check(backend.undo(), "undo restores the previous integrated snapshot");

    bool restoredPosition = false;
    for (const ComponentInstance& component : backend.frontendComponents())
    {
        if (component.label() == "R1")
        {
            restoredPosition = component.position().x == 100.0 &&
                               component.position().y == 0.0;
        }
    }
    check(restoredPosition, "undo restores frontend-visible component state");

    const char* projectPath = "integration_test_project.txt";
    check(backend.save(projectPath), "project saves through FileManager");

    BackendAdapter loaded;
    check(loaded.load(projectPath), "saved project loads through FileManager");
    check(loaded.frontendComponents().size() == 3,
          "loaded backend rebuilds the frontend component list");
    check(loaded.wireViews().size() == 3,
          "loaded project restores wires");

    BackendAdapter analogLed;
    const int ledGround = analogLed.addComponent(7, "GND1", "0V", {0.0, 80.0});
    const int ledBattery = analogLed.addComponent(2, "BAT1", "9V", {0.0, 0.0});
    const int led = analogLed.addComponent(6, "D1", "RED", {100.0, 0.0});
    const int ledResistor = analogLed.addComponent(0, "R1", "1K", {200.0, 0.0});
    check(analogLed.connectPins({ledBattery, 0}, {ledGround, 0}) &&
          analogLed.connectPins({ledBattery, 1}, {led, 0}) &&
          analogLed.connectPins({led, 1}, {ledResistor, 0}) &&
          analogLed.connectPins({ledResistor, 1}, {ledGround, 0}) &&
          analogLed.validate(),
          "battery-LED-resistor circuit is complete and DRC-valid");
    analogLed.startSimulation();
    analogLed.updateSimulation(0.001);
    bool ledCircuitResolved = true;
    for (const WireView& wire : analogLed.wireViews())
    {
        ledCircuitResolved = wire.state != WireLogicState::Floating &&
                             std::isfinite(wire.voltage) &&
                             ledCircuitResolved;
    }
    check(ledCircuitResolved &&
          analogLed.runtimeComponentValue(led).find(" ON") != std::string::npos,
          "LED and every passive node are solved instead of reported floating");
    analogLed.stopSimulation();

    BackendAdapter analogRc;
    const int rcGround = analogRc.addComponent(7, "GND1", "0V", {0.0, 80.0});
    const int rcClock = analogRc.addComponent(3, "CLK1", "1HZ", {0.0, 0.0});
    const int rcResistor = analogRc.addComponent(0, "R1", "1K", {100.0, 0.0});
    const int rcCapacitor = analogRc.addComponent(1, "C1", "100N", {200.0, 0.0});
    check(analogRc.connectPins({rcClock, 1}, {rcGround, 0}) &&
          analogRc.connectPins({rcClock, 0}, {rcResistor, 0}) &&
          analogRc.connectPins({rcResistor, 1}, {rcCapacitor, 0}) &&
          analogRc.connectPins({rcCapacitor, 1}, {rcGround, 0}) &&
          analogRc.validate(),
          "clock-RC circuit is complete and DRC-valid");
    const int rcMeasuredWireId = analogRc.wireViews()[2].id;
    check(analogRc.attachOscilloscopeChannel(0, rcMeasuredWireId),
          "oscilloscope attaches to a passive RC node");
    analogRc.startSimulation();
    const double rcInitialVoltage = analogRc.wireViews()[2].voltage;
    analogRc.updateSimulation(0.001);
    const std::vector<WireView> rcChargedWires = analogRc.wireViews();
    bool rcResolved = rcChargedWires.size() == 4;
    for (const WireView& wire : rcChargedWires)
        rcResolved = wire.state != WireLogicState::Floating &&
                     std::isfinite(wire.voltage) && rcResolved;
    check(rcResolved && rcInitialVoltage < 0.1 &&
          rcChargedWires[2].voltage > 2.0,
          "capacitor follows a finite RC transient without false floating");
    const std::vector<ScopeChannelView> rcScope = analogRc.oscilloscopeChannels();
    check(!rcScope.empty() && !rcScope[0].history.empty() &&
          std::isfinite(rcScope[0].history.back()),
          "oscilloscope records the solved passive-node voltage");
    analogRc.stopSimulation();

    BackendAdapter analogRl;
    const int rlGround = analogRl.addComponent(7, "GND1", "0V", {0.0, 80.0});
    const int rlSource = analogRl.addComponent(9, "V1", "5V", {0.0, 0.0});
    const int rlInductor = analogRl.addComponent(8, "L1", "10M", {100.0, 0.0});
    const int rlResistor = analogRl.addComponent(0, "R1", "100", {200.0, 0.0});
    check(analogRl.connectPins({rlSource, 0}, {rlGround, 0}) &&
          analogRl.connectPins({rlSource, 1}, {rlInductor, 0}) &&
          analogRl.connectPins({rlInductor, 1}, {rlResistor, 0}) &&
          analogRl.connectPins({rlResistor, 1}, {rlGround, 0}) &&
          analogRl.validate(),
          "DC-source RL circuit is complete and DRC-valid");
    analogRl.startSimulation();
    analogRl.updateSimulation(0.001);
    const std::vector<WireView> rlWires = analogRl.wireViews();
    bool rlResolved = rlWires.size() == 4;
    for (const WireView& wire : rlWires)
        rlResolved = wire.state != WireLogicState::Floating &&
                     std::isfinite(wire.voltage) && rlResolved;
    check(rlResolved && rlWires[2].voltage > 0.0 &&
          rlWires[2].voltage < 5.0,
          "inductor follows a finite RL transient and resolves both terminals");
    analogRl.stopSimulation();

    BackendAdapter adcCircuit;
    const int adcGround = adcCircuit.addComponent(7, "GND1", "0V", {0.0, 100.0});
    const int adcReference = adcCircuit.addComponent(9, "VREF", "5V", {0.0, 0.0});
    const int adcInput = adcCircuit.addComponent(9, "VIN", "2.5V", {0.0, 50.0});
    const int adc = adcCircuit.addComponent(17, "ADC1", "4 BIT", {120.0, 20.0});
    check(adcCircuit.connectPins({adcReference, 0}, {adcGround, 0}) &&
          adcCircuit.connectPins({adcReference, 1}, {adc, 1}) &&
          adcCircuit.connectPins({adcInput, 0}, {adcGround, 0}) &&
          adcCircuit.connectPins({adcInput, 1}, {adc, 0}) &&
          adcCircuit.connectPins({adc, 2}, {adcGround, 0}) &&
          adcCircuit.validate(),
          "ADC input and both references are wired to solved analog nets");
    adcCircuit.startSimulation();
    adcCircuit.updateSimulation(0.001);
    check(adcCircuit.runtimeComponentValue(adc) == "CODE 8",
          "ADC converts a live 2.5 V net to the expected 4-bit code");
    adcCircuit.stopSimulation();

    BackendAdapter dacCircuit;
    const int dacGround = dacCircuit.addComponent(7, "GND1", "0V", {0.0, 120.0});
    const int dacReference = dacCircuit.addComponent(9, "VREF", "5V", {0.0, 0.0});
    const int dac = dacCircuit.addComponent(18, "DAC1", "4 BIT", {160.0, 20.0});
    const int bit0 = dacCircuit.addComponent(10, "B0", "RELEASED", {0.0, 20.0});
    const int bit1 = dacCircuit.addComponent(10, "B1", "PRESSED", {0.0, 40.0});
    const int bit2 = dacCircuit.addComponent(10, "B2", "RELEASED", {0.0, 60.0});
    const int bit3 = dacCircuit.addComponent(10, "B3", "PRESSED", {0.0, 80.0});
    const int dacLoad = dacCircuit.addComponent(0, "RLOAD", "1K", {280.0, 20.0});
    check(dacCircuit.connectPins({dacReference, 0}, {dacGround, 0}) &&
          dacCircuit.connectPins({dacReference, 1}, {dac, 4}) &&
          dacCircuit.connectPins({dac, 5}, {dacGround, 0}) &&
          dacCircuit.connectPins({bit0, 0}, {dac, 0}) &&
          dacCircuit.connectPins({bit1, 0}, {dac, 1}) &&
          dacCircuit.connectPins({bit2, 0}, {dac, 2}) &&
          dacCircuit.connectPins({bit3, 0}, {dac, 3}) &&
          dacCircuit.connectPins({dac, 6}, {dacLoad, 0}) &&
          dacCircuit.connectPins({dacLoad, 1}, {dacGround, 0}) &&
          dacCircuit.validate(),
          "DAC data, references and analog load are fully wired");
    dacCircuit.startSimulation();
    dacCircuit.updateSimulation(0.001);
    const double dacVoltage = std::stod(dacCircuit.runtimeComponentValue(dac));
    bool dacNetsResolved = true;
    for (const WireView& wire : dacCircuit.wireViews())
        dacNetsResolved = wire.state != WireLogicState::Floating &&
                          std::isfinite(wire.voltage) && dacNetsResolved;
    check(dacNetsResolved && dacVoltage > 3.2 && dacVoltage < 3.5,
          "DAC code 0b1010 drives the loaded analog net near 3.33 V");
    dacCircuit.stopSimulation();

    BackendAdapter digitalCircuit;
    const int digitalGround = digitalCircuit.addComponent(7, "GND1", "0V", {0.0, 160.0});
    const int digitalHigh = digitalCircuit.addComponent(10, "HIGH", "PRESSED", {0.0, 0.0});
    const int digitalLow = digitalCircuit.addComponent(10, "LOW", "RELEASED", {0.0, 30.0});
    const int digitalClock = digitalCircuit.addComponent(3, "CLK1", "1KHZ", {0.0, 60.0});
    const int andGate = digitalCircuit.addComponent(5, "AND1", "2 INPUTS", {100.0, 0.0});
    const int orGate = digitalCircuit.addComponent(12, "OR1", "2 INPUTS", {100.0, 40.0});
    const int xorGate = digitalCircuit.addComponent(14, "XOR1", "2 INPUTS", {100.0, 80.0});
    const int nandGate = digitalCircuit.addComponent(15, "NAND1", "2 INPUTS", {100.0, 120.0});
    const int notGate = digitalCircuit.addComponent(13, "NOT1", "INVERTER", {100.0, 160.0});
    const int flipFlop = digitalCircuit.addComponent(16, "FF1", "D TYPE", {100.0, 200.0});
    const int segment = digitalCircuit.addComponent(11, "SEG1", "0", {240.0, 80.0});
    const bool digitalWired =
          digitalCircuit.connectPins({digitalClock, 1}, {digitalGround, 0}) &&
          digitalCircuit.connectPins({digitalHigh, 0}, {andGate, 0}) &&
          digitalCircuit.connectPins({digitalLow, 0}, {andGate, 1}) &&
          digitalCircuit.connectPins({digitalHigh, 0}, {orGate, 0}) &&
          digitalCircuit.connectPins({digitalLow, 0}, {orGate, 1}) &&
          digitalCircuit.connectPins({digitalHigh, 0}, {xorGate, 0}) &&
          digitalCircuit.connectPins({digitalLow, 0}, {xorGate, 1}) &&
          digitalCircuit.connectPins({digitalHigh, 0}, {nandGate, 0}) &&
          digitalCircuit.connectPins({digitalHigh, 0}, {nandGate, 1}) &&
          digitalCircuit.connectPins({digitalLow, 0}, {notGate, 0}) &&
          digitalCircuit.connectPins({digitalHigh, 0}, {flipFlop, 0}) &&
          digitalCircuit.connectPins({digitalClock, 0}, {flipFlop, 1}) &&
          digitalCircuit.connectPins({andGate, 2}, {segment, 0}) &&
          digitalCircuit.connectPins({orGate, 2}, {segment, 1}) &&
          digitalCircuit.connectPins({xorGate, 2}, {segment, 2}) &&
          digitalCircuit.connectPins({nandGate, 2}, {segment, 3}) &&
          digitalCircuit.connectPins({notGate, 1}, {segment, 4}) &&
          digitalCircuit.connectPins({flipFlop, 2}, {segment, 5}) &&
          digitalCircuit.connectPins({digitalGround, 0}, {segment, 6}) &&
          digitalCircuit.connectPins({digitalHigh, 0}, {segment, 7});
    check(digitalWired && digitalCircuit.validate(),
          "all logic gates, D flip-flop and seven-segment inputs pass DRC");
    digitalCircuit.startSimulation();
    digitalCircuit.updateSimulation(0.001);
    check(digitalCircuit.runtimeComponentValue(segment) == "SEG ON:5",
          "mixed simulation settles AND/OR/XOR/NAND/NOT/DFF into seven-segment state");
    digitalCircuit.stopSimulation();

    BackendAdapter meters;
    const int meterGround = meters.addComponent(7, "GND1", "0V", {0.0, 80.0});
    const int meterBattery = meters.addComponent(2, "BAT1", "9V", {0.0, 0.0});
    const int ammeter = meters.addComponent(24, "AM1", "AUTO", {80.0, 0.0});
    const int meterResistor = meters.addComponent(0, "R1", "1K", {160.0, 0.0});
    const int voltmeter = meters.addComponent(23, "VM1", "AUTO", {160.0, 60.0});
    check(meters.connectPins({meterBattery, 0}, {meterGround, 0}) &&
          meters.connectPins({meterBattery, 1}, {ammeter, 0}) &&
          meters.connectPins({ammeter, 1}, {meterResistor, 0}) &&
          meters.connectPins({meterResistor, 1}, {meterGround, 0}) &&
          meters.connectPins({voltmeter, 0}, {meterResistor, 0}) &&
          meters.connectPins({voltmeter, 1}, {meterGround, 0}) &&
          meters.validate(),
          "ammeter and voltmeter circuit is wired and referenced");
    meters.startSimulation();
    meters.updateSimulation(0.001);
    check(std::stod(meters.runtimeComponentValue(ammeter)) > 0.008 &&
          std::stod(meters.runtimeComponentValue(voltmeter)) > 8.9,
          "measurement components expose live solved current and voltage");
    meters.stopSimulation();

    BackendAdapter interactive;
    const int liveButton = interactive.addComponent(
            10, "PB1", "RELEASED", {0.0, 0.0});
    const int liveSwitch = interactive.addComponent(
            4, "SW1", "OPEN", {100.0, 0.0});
    const int liveResistor = interactive.addComponent(
            0, "R1", "1K", {200.0, 0.0});
    const int liveGround = interactive.addComponent(
            7, "GND1", "0V", {200.0, 80.0});
    check(interactive.connectPins({liveButton, 0}, {liveSwitch, 0}) &&
          interactive.connectPins({liveSwitch, 1}, {liveResistor, 0}) &&
          interactive.connectPins({liveResistor, 1}, {liveGround, 0}),
          "interactive Section 8 test circuit is wired");
    check(interactive.validate(),
          "interactive Section 8 circuit passes DRC before Run");
    const int switchedWireId = interactive.wireViews()[1].id;
    interactive.startSimulation();
    check(interactive.setPushButtonPressed(liveButton, true) &&
          interactive.toggleSwitch(liveSwitch),
          "push button and switch accept live interaction while running");
    WireLogicState switchedState = WireLogicState::Floating;
    for (const WireView& wire : interactive.wireViews())
    {
        if (wire.id == switchedWireId)
        {
            switchedState = wire.state;
        }
    }
    check(switchedState == WireLogicState::High,
          "live interaction propagates HIGH across the complete switched net");
    interactive.pauseSimulation();
    check(interactive.setPushButtonPressed(liveButton, false),
          "interactive components remain usable while paused");
    for (const WireView& wire : interactive.wireViews())
    {
        if (wire.id == switchedWireId)
        {
            switchedState = wire.state;
        }
    }
    check(switchedState == WireLogicState::Low,
          "paused live interaction immediately updates the wire to LOW");
    const std::string originalResistance =
            interactive.runtimeComponentValue(liveResistor);
    check(interactive.adjustInteractiveValue(liveResistor, 1) &&
          interactive.runtimeComponentValue(liveResistor) != originalResistance,
          "component values can be adjusted live during simulation");
    interactive.stopSimulation();
    check(interactive.runtimeComponentValue(liveButton) == "RELEASED" &&
          interactive.runtimeComponentValue(liveSwitch) == "OPEN" &&
          interactive.runtimeComponentValue(liveResistor) == originalResistance,
          "Stop restores the editable pre-simulation component snapshot");

    BackendAdapter catalog;
    const std::vector<std::string> defaultValues = {
        "1K", "100N", "9V", "1HZ", "OPEN", "2 INPUTS", "RED", "0V",
        "1M", "5V", "RELEASED", "0", "2 INPUTS", "INVERTER",
        "2 INPUTS", "2 INPUTS", "D TYPE", "4 BIT", "4 BIT", "1US",
        "8 BIT", "16X2", "4X4", "AUTO", "AUTO"
    };
    bool completeCatalog = true;
    for (int definitionId = 0;
         definitionId < static_cast<int>(defaultValues.size());
         ++definitionId)
    {
        completeCatalog = catalog.addComponent(
                definitionId,
                "X" + std::to_string(definitionId),
                defaultValues[definitionId],
                {static_cast<double>(definitionId * 20), 200.0}) >= 0 &&
                completeCatalog;
    }
    check(completeCatalog && catalog.frontendComponents().size() == 25,
          "all integrated component definitions create backend objects");

    const char* catalogPath = "integration_catalog_project.txt";
    check(catalog.save(catalogPath), "complete component catalog saves");
    BackendAdapter loadedCatalog;
    check(loadedCatalog.load(catalogPath) &&
          loadedCatalog.frontendComponents().size() == 25,
          "complete component catalog loads through the integration layer");

    std::remove(projectPath);
    std::remove(catalogPath);
    std::remove("recent_projects.txt");

    std::cout << "Integration failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
