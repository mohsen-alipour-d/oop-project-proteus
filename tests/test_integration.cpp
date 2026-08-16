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
    check(backend.connectPins({source, 0}, {resistor, 0}),
          "first wire is connected");
    check(backend.connectPins({ground, 0}, {resistor, 1}),
          "second wire is connected");
    check(backend.wireViews().size() == 2,
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
    check(loaded.wireViews().size() == 2,
          "loaded project restores wires");

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
