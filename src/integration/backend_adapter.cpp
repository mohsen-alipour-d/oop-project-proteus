#include "backend_adapter.h"

#include "../components/advanced/adc.h"
#include "../components/advanced/dac.h"
#include "../components/advanced/external_memory.h"
#include "../components/advanced/lcd16x2.h"
#include "../components/advanced/matrix_keypad.h"
#include "../components/advanced/microcontroller.h"
#include "../components/digital/d_flipflop.h"
#include "../components/digital/logic_gates.h"
#include "../components/interactive/led.h"
#include "../components/interactive/push_button.h"
#include "../components/interactive/seven_segment.h"
#include "../components/interactive/switch_comp.h"
#include "../components/passive/capacitor.h"
#include "../components/passive/inductor.h"
#include "../components/passive/resistor.h"
#include "../components/sources/battery.h"
#include "../components/sources/clock_gen.h"
#include "../components/sources/dc_source.h"
#include "../components/sources/ground.h"
#include "../measurement/ammeter.h"
#include "../measurement/voltmeter.h"
#include "../simulation/analog_solver.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
double leadingNumber(const std::string& text, double fallback)
{
    const std::string normalized = toUpperAscii(trimAscii(text));
    std::size_t length = 0;
    while (length < normalized.size())
    {
        const char character = normalized[length];
        if (!std::isdigit(static_cast<unsigned char>(character)) &&
            character != '.' && character != '-' && character != '+')
        {
            break;
        }
        ++length;
    }

    if (length == 0)
    {
        return fallback;
    }

    try
    {
        return std::stod(normalized.substr(0, length));
    }
    catch (const std::exception&)
    {
        return fallback;
    }
}

double resistanceValue(const std::string& text, double fallback)
{
    const std::string normalized = toUpperAscii(trimAscii(text));
    double value = leadingNumber(normalized, fallback);
    if (normalized.find("MEG") != std::string::npos)
    {
        return value * 1.0e6;
    }
    if (normalized.find('K') != std::string::npos)
    {
        return value * 1.0e3;
    }
    if (normalized.find('M') != std::string::npos)
    {
        return value * 1.0e6;
    }
    return value;
}

double reactiveValue(const std::string& text, double fallback)
{
    const std::string normalized = toUpperAscii(trimAscii(text));
    double value = leadingNumber(normalized, fallback);
    if (normalized.find('P') != std::string::npos)
    {
        return value * 1.0e-12;
    }
    if (normalized.find('N') != std::string::npos)
    {
        return value * 1.0e-9;
    }
    if (normalized.find('U') != std::string::npos)
    {
        return value * 1.0e-6;
    }
    if (normalized.find('M') != std::string::npos)
    {
        return value * 1.0e-3;
    }
    return value;
}

double timeValue(const std::string& text, double fallback)
{
    const std::string normalized = toUpperAscii(trimAscii(text));
    double value = leadingNumber(normalized, fallback);
    if (normalized.find("NS") != std::string::npos)
    {
        return value * 1.0e-9;
    }
    if (normalized.find("US") != std::string::npos)
    {
        return value * 1.0e-6;
    }
    if (normalized.find("MS") != std::string::npos)
    {
        return value * 1.0e-3;
    }
    return value;
}

int bitCountValue(const std::string& text, int fallback, int maximum)
{
    const int parsed = static_cast<int>(std::lround(leadingNumber(text, fallback)));
    return std::clamp(parsed, 1, maximum);
}

std::string compactNumber(double value)
{
    std::ostringstream output;
    output << std::setprecision(6) << std::defaultfloat << value;
    return output.str();
}

std::string lowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character)
                   {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}
}

BackendAdapter::BackendAdapter()
{
    resetProject();
}

void BackendAdapter::resetProject()
{
    circuit.clear();
    fileManager = FileManager{};
    history = History{};
    drc.log.clear();
    oscilloscope = Oscilloscope{};
    simulationController = SimulationController{};
    simulationBaseline.clear();
    recordHistory();
}

int BackendAdapter::addComponent(int definitionId,
                                 const std::string& label,
                                 const std::string& value,
                                 const WorldPoint& position)
{
    if (!isLabelAvailable(label, -1))
    {
        return -1;
    }

    Component* component = createBackendComponent(definitionId,
                                                   label,
                                                   value,
                                                   position);
    if (component == nullptr)
    {
        return -1;
    }

    applyDefinitionGeometry(*component, definitionId);
    circuit.addComponent(component);
    return component->id;
}

bool BackendAdapter::syncComponent(const ComponentInstance& frontend)
{
    Component* backend = componentById(frontend.id());
    if (backend == nullptr ||
        !isLabelAvailable(frontend.label(), frontend.id()))
    {
        return false;
    }

    backend->name = frontend.label();
    backend->position = {frontend.position().x, frontend.position().y};
    backend->rotation = frontend.rotationDegrees();
    backend->mirroredH = frontend.isMirroredHorizontally();
    backend->mirroredV = frontend.isMirroredVertically();
    applyEditableValue(*backend, frontend.definitionId(), frontend.value());
    circuit.refreshWires();
    return true;
}

bool BackendAdapter::removeComponent(int componentId)
{
    Component* component = componentById(componentId);
    if (component == nullptr)
    {
        return false;
    }
    circuit.removeComponent(component);
    return true;
}

bool BackendAdapter::isLabelAvailable(const std::string& label,
                                      int exceptId) const
{
    const std::string normalized = trimAscii(label);
    if (normalized.empty() || normalized.find_first_of(" \t\r\n") != std::string::npos)
    {
        return false;
    }
    for (Component* component : circuit.components)
    {
        if (component->id != exceptId && component->name == normalized)
        {
            return false;
        }
    }
    return true;
}

std::optional<PinHandle> BackendAdapter::findPinAt(const WorldPoint& point,
                                                   double tolerance) const
{
    const Vector2D position{point.x, point.y};
    for (auto component = circuit.components.rbegin();
         component != circuit.components.rend(); ++component)
    {
        for (int index = 0; index < static_cast<int>((*component)->pins.size()); ++index)
        {
            if ((*component)->pins[index].worldPos().distanceTo(position) <= tolerance)
            {
                return PinHandle{(*component)->id, index};
            }
        }
    }
    return std::nullopt;
}

std::optional<WorldPoint> BackendAdapter::pinPosition(const PinHandle& handle) const
{
    Pin* pin = pinFromHandle(handle);
    if (pin == nullptr)
    {
        return std::nullopt;
    }
    const Vector2D position = pin->worldPos();
    return WorldPoint{position.x, position.y};
}

bool BackendAdapter::connectPins(const PinHandle& first,
                                 const PinHandle& second)
{
    Pin* firstPin = pinFromHandle(first);
    Pin* secondPin = pinFromHandle(second);
    if (firstPin == nullptr || secondPin == nullptr || firstPin == secondPin)
    {
        return false;
    }

    for (Wire* wire : circuit.wires)
    {
        const bool sameDirection = wire->startPin == firstPin && wire->endPin == secondPin;
        const bool reverseDirection = wire->startPin == secondPin && wire->endPin == firstPin;
        if (sameDirection || reverseDirection)
        {
            return false;
        }
    }

    circuit.addWire(firstPin, secondPin);
    return true;
}

int BackendAdapter::findWireAt(const WorldPoint& point, double tolerance) const
{
    const Vector2D position{point.x, point.y};
    for (auto wire = circuit.wires.rbegin(); wire != circuit.wires.rend(); ++wire)
    {
        if ((*wire)->containsPoint(position, tolerance))
        {
            return (*wire)->id;
        }
    }
    return -1;
}

bool BackendAdapter::removeWire(int wireId)
{
    Wire* wire = wireById(wireId);
    if (wire == nullptr)
    {
        return false;
    }
    circuit.removeWire(wire);
    return true;
}

bool BackendAdapter::addJunctionAt(const WorldPoint& point)
{
    return circuit.addJunctionAt({point.x, point.y}) != nullptr;
}

std::vector<WireView> BackendAdapter::wireViews() const
{
    std::vector<WireView> result;
    result.reserve(circuit.wires.size());
    for (Wire* wire : circuit.wires)
    {
        WireView view;
        view.id = wire->id;
        for (const Vector2D& point : wire->points)
        {
            view.points.push_back({point.x, point.y});
        }

        if (simulationController.active() &&
            wire->netId >= 0 &&
            wire->netId < static_cast<int>(circuit.nets.size()))
        {
            Net* net = circuit.nets[wire->netId];
            view.voltage = net->voltage;
            view.voltageResolved = net->voltageResolved;
            if (!net->voltageResolved)
            {
                result.push_back(view);
                continue;
            }
            if (net->driverConflict)
            {
                view.state = WireLogicState::Undefined;
                result.push_back(view);
                continue;
            }
            switch (net->logicState())
            {
                case LOW:
                    view.state = WireLogicState::Low;
                    break;
                case HIGH:
                    view.state = WireLogicState::High;
                    break;
                case UNDEFINED:
                    view.state = WireLogicState::Undefined;
                    break;
            }
        }
        result.push_back(view);
    }
    return result;
}

std::vector<WorldPoint> BackendAdapter::junctionPositions() const
{
    std::vector<WorldPoint> result;
    result.reserve(circuit.junctions.size());
    for (Junction* junction : circuit.junctions)
    {
        result.push_back({junction->position.x, junction->position.y});
    }
    return result;
}

bool BackendAdapter::attachOscilloscopeChannel(int channelIndex, int wireId)
{
    Wire* wire = wireById(wireId);
    if (wire == nullptr || wire->netId < 0 ||
        channelIndex < 0 ||
        channelIndex >= static_cast<int>(oscilloscope.channels.size()))
    {
        return false;
    }
    oscilloscope.attachChannel(channelIndex, wire->netId);
    return true;
}

void BackendAdapter::startOscilloscope()
{
    oscilloscope.start();
}

void BackendAdapter::pauseOscilloscope()
{
    oscilloscope.pause();
}

std::vector<ScopeChannelView> BackendAdapter::oscilloscopeChannels() const
{
    std::vector<ScopeChannelView> result;
    result.reserve(oscilloscope.channels.size());
    for (int index = 0;
         index < static_cast<int>(oscilloscope.channels.size());
         ++index)
    {
        const ScopeChannel& channel = oscilloscope.channels[index];
        result.push_back({index,
                          channel.netId,
                          channel.timeDiv,
                          channel.voltDiv,
                          channel.history});
    }
    return result;
}

void BackendAdapter::recordHistory()
{
    const std::string snapshot = fileManager.serialize(circuit);
    if (history.current >= 0 &&
        history.current < static_cast<int>(history.snapshots.size()) &&
        history.snapshots[history.current] == snapshot)
    {
        return;
    }
    history.push(snapshot);
}

bool BackendAdapter::undo()
{
    if (!history.canUndo())
    {
        return false;
    }
    fileManager.deserialize(circuit, history.undo());
    oscilloscope = Oscilloscope{};
    for (Component* component : circuit.components)
    {
        applyDefinitionGeometry(*component, definitionIdFor(*component));
    }
    circuit.refreshWires();
    simulationController = SimulationController{};
    simulationBaseline.clear();
    return true;
}

bool BackendAdapter::redo()
{
    if (!history.canRedo())
    {
        return false;
    }
    fileManager.deserialize(circuit, history.redo());
    oscilloscope = Oscilloscope{};
    for (Component* component : circuit.components)
    {
        applyDefinitionGeometry(*component, definitionIdFor(*component));
    }
    circuit.refreshWires();
    simulationController = SimulationController{};
    simulationBaseline.clear();
    return true;
}

bool BackendAdapter::canUndo() const
{
    return history.current > 0;
}

bool BackendAdapter::canRedo() const
{
    return history.current >= 0 &&
           history.current < static_cast<int>(history.snapshots.size()) - 1;
}

bool BackendAdapter::save(const std::string& defaultPath)
{
    if (fileManager.saved && !fileManager.filePath.empty())
    {
        return fileManager.save(circuit);
    }
    return fileManager.saveAs(circuit, defaultPath);
}

bool BackendAdapter::load(const std::string& path)
{
    if (!fileManager.load(circuit, path))
    {
        return false;
    }
    for (Component* component : circuit.components)
    {
        applyDefinitionGeometry(*component, definitionIdFor(*component));
    }
    circuit.refreshWires();
    history = History{};
    oscilloscope = Oscilloscope{};
    recordHistory();
    simulationController = SimulationController{};
    simulationBaseline.clear();
    return true;
}

bool BackendAdapter::loadMostRecent()
{
    fileManager.loadRecentList();
    if (fileManager.recentProjects.empty())
    {
        return false;
    }
    return load(fileManager.recentProjects.front());
}

std::string BackendAdapter::currentPath() const
{
    return fileManager.filePath;
}

std::vector<ComponentInstance> BackendAdapter::frontendComponents() const
{
    std::vector<ComponentInstance> result;
    result.reserve(circuit.components.size());
    for (Component* component : circuit.components)
    {
        const int definitionId = definitionIdFor(*component);
        if (definitionId < 0)
        {
            continue;
        }
        ComponentInstance frontend(component->id,
                                   definitionId,
                                   component->name,
                                   displayValueFor(*component, definitionId),
                                   {component->position.x, component->position.y});
        frontend.setTransform(component->rotation,
                              component->mirroredH,
                              component->mirroredV);
        result.push_back(frontend);
    }
    return result;
}

bool BackendAdapter::validate()
{
    return drc.validate(circuit);
}

const std::vector<LogMessage>& BackendAdapter::validationMessages() const
{
    return drc.log.messages;
}

void BackendAdapter::step(double dt)
{
    if (dt <= 0.0)
    {
        return;
    }

    // Compatibility entry point for older tests/clients.
    if (simulationController.state() != SimulationState::Running)
    {
        startSimulation();
    }
    updateSimulation(dt);
}

void BackendAdapter::startSimulation()
{
    prepareSimulationSession();
    simulationController.run();
    oscilloscope.start();
    evaluateCircuit(0.0);
}

void BackendAdapter::pauseSimulation()
{
    simulationController.pause();
    oscilloscope.pause();
}

void BackendAdapter::updateSimulation(double dt)
{
    const double advanced = simulationController.tick(dt);
    if (advanced > 0.0)
    {
        evaluateCircuit(advanced);
    }
}

bool BackendAdapter::stepSimulation()
{
    prepareSimulationSession();
    oscilloscope.start();
    const double advanced = simulationController.stepOnce();
    evaluateCircuit(advanced);
    oscilloscope.pause();
    return advanced > 0.0;
}

void BackendAdapter::stopSimulation()
{
    if (!simulationBaseline.empty())
    {
        fileManager.deserialize(circuit, simulationBaseline);
        for (Component* component : circuit.components)
        {
            applyDefinitionGeometry(*component, definitionIdFor(*component));
        }
        circuit.refreshWires();
    }

    simulationController.stop();
    simulationBaseline.clear();
    oscilloscope.stop();
}

SimulationState BackendAdapter::simulationState() const
{
    return simulationController.state();
}

double BackendAdapter::simulationTimeSeconds() const
{
    return simulationController.timeSeconds();
}

double BackendAdapter::simulationStepSeconds() const
{
    return simulationController.stepSizeSeconds();
}

bool BackendAdapter::toggleSwitch(int componentId)
{
    if (!simulationController.active())
    {
        return false;
    }

    Component* component = componentById(componentId);
    if (component == nullptr || definitionIdFor(*component) != 4)
    {
        return false;
    }

    static_cast<Switch&>(*component).toggle();
    evaluateCircuit(0.0);
    return true;
}

bool BackendAdapter::setPushButtonPressed(int componentId, bool pressed)
{
    if (!simulationController.active())
    {
        return false;
    }

    Component* component = componentById(componentId);
    if (component == nullptr || definitionIdFor(*component) != 10)
    {
        return false;
    }

    PushButton& button = static_cast<PushButton&>(*component);
    if (pressed)
    {
        button.press();
    }
    else
    {
        button.release();
    }
    evaluateCircuit(0.0);
    return true;
}

bool BackendAdapter::adjustInteractiveValue(int componentId, int direction)
{
    if (!simulationController.active() || direction == 0)
    {
        return false;
    }

    Component* component = componentById(componentId);
    if (component == nullptr)
    {
        return false;
    }

    const int definitionId = definitionIdFor(*component);
    if (definitionId == 0)
    {
        Resistor& resistor = static_cast<Resistor&>(*component);
        const double factor = direction > 0 ? 1.1 : 1.0 / 1.1;
        resistor.value = std::clamp(resistor.value * factor, 1.0, 1.0e9);
    }
    else if (definitionId == 9)
    {
        DCSource& source = static_cast<DCSource&>(*component);
        source.value = std::clamp(source.value +
                                  (direction > 0 ? 0.5 : -0.5),
                                  -1000.0,
                                  1000.0);
    }
    else
    {
        return false;
    }

    evaluateCircuit(0.0);
    return true;
}

std::string BackendAdapter::runtimeComponentValue(int componentId) const
{
    Component* component = componentById(componentId);
    if (component == nullptr)
    {
        return {};
    }
    const int definitionId = definitionIdFor(*component);
    switch (definitionId)
    {
        case 6:
        {
            const LED& led = static_cast<const LED&>(*component);
            return toUpperAscii(led.color) + (led.lit ? " ON" : " OFF");
        }
        case 11:
        {
            const SevenSegment& display = static_cast<const SevenSegment&>(*component);
            int activeSegments = 0;
            for (bool segment : display.segOn)
                activeSegments += segment ? 1 : 0;
            return "SEG ON:" + std::to_string(activeSegments);
        }
        case 17:
            return "CODE " + std::to_string(
                    static_cast<const ADC&>(*component).getOutputCode());
        case 18:
            return compactNumber(
                    static_cast<const DAC&>(*component).getOutputVoltage()) + "V";
        case 23:
        {
            const Voltmeter& meter = static_cast<const Voltmeter&>(*component);
            return meter.hasError ? "ERR" : compactNumber(meter.reading) + "V";
        }
        case 24:
            return compactNumber(
                    static_cast<const Ammeter&>(*component).reading) + "A";
        default:
            return displayValueFor(*component, definitionId);
    }
}

void BackendAdapter::prepareSimulationSession()
{
    if (simulationController.state() == SimulationState::Stopped)
    {
        simulationBaseline = fileManager.serialize(circuit);
    }
}

void BackendAdapter::evaluateCircuit(double dt)
{
    // Several zero-time settling passes make cascaded combinational changes
    // visible immediately without advancing the simulation clock again.
    constexpr int SETTLING_PASSES = 4;
    for (int pass = 0; pass < SETTLING_PASSES; ++pass)
    {
        for (Component* component : circuit.components)
        {
            if (!AnalogSolver::ownsComponentStep(*component))
            {
                component->step(pass == 0 ? dt : 0.0,
                                simulationController.timeSeconds());
            }
        }
        circuit.propagateVoltages();
        AnalogSolver::solve(circuit,
                            pass == 0 ? dt : 0.0,
                            pass == 0 && dt > 0.0);
        circuit.propagateVoltages();
    }
    oscilloscope.update(circuit, simulationController.timeSeconds());
}

Component* BackendAdapter::componentById(int componentId) const
{
    for (Component* component : circuit.components)
    {
        if (component->id == componentId)
        {
            return component;
        }
    }
    return nullptr;
}

Wire* BackendAdapter::wireById(int wireId) const
{
    for (Wire* wire : circuit.wires)
    {
        if (wire->id == wireId)
        {
            return wire;
        }
    }
    return nullptr;
}

Pin* BackendAdapter::pinFromHandle(const PinHandle& handle) const
{
    Component* component = componentById(handle.componentId);
    if (component == nullptr || handle.pinIndex < 0 ||
        handle.pinIndex >= static_cast<int>(component->pins.size()))
    {
        return nullptr;
    }
    return &component->pins[handle.pinIndex];
}

Component* BackendAdapter::createBackendComponent(int definitionId,
                                                  const std::string& label,
                                                  const std::string& value,
                                                  const WorldPoint& position) const
{
    const double x = position.x;
    const double y = position.y;
    switch (definitionId)
    {
        case 0: return new Resistor(label, x, y,
                                    std::max(resistanceValue(value, 1000.0),
                                             1.0e-9));
        case 1: return new Capacitor(label, x, y,
                                     std::max(reactiveValue(value, 100.0e-9),
                                              1.0e-15));
        case 2: return new Battery(label, x, y, leadingNumber(value, 9.0));
        case 3:
        {
            const double frequency = std::max(leadingNumber(value, 1.0), 1.0e-9);
            return new ClockGenerator(label, x, y, 1.0 / frequency);
        }
        case 4:
        {
            auto* component = new Switch(label, x, y);
            component->closed = toUpperAscii(value).find("CLOSED") != std::string::npos;
            return component;
        }
        case 5: return new AndGate(label, x, y, 2);
        case 6: return new LED(label, x, y, lowerAscii(trimAscii(value)));
        case 7: return new Ground(label, x, y);
        case 8: return new Inductor(label, x, y,
                                    std::max(reactiveValue(value, 1.0e-3),
                                             1.0e-15));
        case 9: return new DCSource(label, x, y, leadingNumber(value, 5.0));
        case 10:
        {
            auto* component = new PushButton(label, x, y);
            component->pressed = toUpperAscii(value).find("PRESSED") != std::string::npos &&
                                 toUpperAscii(value).find("RELEASED") == std::string::npos;
            return component;
        }
        case 11: return new SevenSegment(label, x, y);
        case 12: return new OrGate(label, x, y, 2);
        case 13: return new NotGate(label, x, y);
        case 14: return new XorGate(label, x, y, 2);
        case 15: return new NandGate(label, x, y, 2);
        case 16: return new DFlipFlop(label, x, y);
        case 17: return new ADC(label, x, y, bitCountValue(value, 4, 31), 1.0e-6);
        case 18: return new DAC(label, x, y, bitCountValue(value, 4, 31), 1.0e-6);
        case 19: return new Microcontroller(label, x, y, timeValue(value, 1.0e-6));
        case 20: return new ExternalMemory(label, x, y, bitCountValue(value, 8, 16));
        case 21: return new LCD16x2(label, x, y);
        case 22: return new MatrixKeypad(label, x, y);
        case 23: return new Voltmeter(label, x, y);
        case 24: return new Ammeter(label, x, y);
        default: return nullptr;
    }
}

int BackendAdapter::definitionIdFor(const Component& component) const
{
    const std::string tag = component.typeTag();
    if (tag == "R") return 0;
    if (tag == "C") return 1;
    if (tag == "B") return 2;
    if (tag == "K") return 3;
    if (tag == "S") return 4;
    if (tag == "A") return 5;
    if (tag == "D") return 6;
    if (tag == "G") return 7;
    if (tag == "L") return 8;
    if (tag == "V") return 9;
    if (tag == "P") return 10;
    if (tag == "E") return 11;
    if (tag == "O") return 12;
    if (tag == "N") return 13;
    if (tag == "X") return 14;
    if (tag == "Q") return 15;
    if (tag == "F") return 16;
    if (tag == "ADC") return 17;
    if (tag == "DAC") return 18;
    if (tag == "MCU") return 19;
    if (tag == "MEM") return 20;
    if (tag == "LCD") return 21;
    if (tag == "KEYPAD") return 22;
    if (tag == "M") return 23;
    if (tag == "Y") return 24;
    return -1;
}

std::string BackendAdapter::displayValueFor(const Component& component,
                                            int definitionId) const
{
    switch (definitionId)
    {
        case 0: return compactNumber(static_cast<const Resistor&>(component).value);
        case 1: return compactNumber(static_cast<const Capacitor&>(component).value);
        case 2: return compactNumber(static_cast<const Battery&>(component).value) + "V";
        case 3:
        {
            const double period = static_cast<const ClockGenerator&>(component).period;
            return compactNumber(period > 0.0 ? 1.0 / period : 0.0) + "HZ";
        }
        case 4: return static_cast<const Switch&>(component).closed ? "CLOSED" : "OPEN";
        case 5: return "2 INPUTS";
        case 6: return toUpperAscii(static_cast<const LED&>(component).color);
        case 7: return "0V";
        case 8: return compactNumber(static_cast<const Inductor&>(component).value);
        case 9: return compactNumber(static_cast<const DCSource&>(component).value) + "V";
        case 10: return static_cast<const PushButton&>(component).pressed ? "PRESSED" : "RELEASED";
        case 11: return "0";
        case 12: return "2 INPUTS";
        case 13: return "INVERTER";
        case 14: return "2 INPUTS";
        case 15: return "2 INPUTS";
        case 16: return "D TYPE";
        case 17: return std::to_string(static_cast<const ADC&>(component).getBitCount()) + " BIT";
        case 18: return std::to_string(static_cast<const DAC&>(component).getBitCount()) + " BIT";
        case 19: return compactNumber(static_cast<const Microcontroller&>(component).getInstructionPeriod()) + "S";
        case 20: return std::to_string(static_cast<const ExternalMemory&>(component).getAddressBits()) + " BIT";
        case 21: return "16X2";
        case 22: return "4X4";
        case 23: return "AUTO";
        case 24: return "AUTO";
        default: return "";
    }
}

void BackendAdapter::applyDefinitionGeometry(Component& component,
                                             int definitionId) const
{
    const ComponentDefinition* definition = definitions.findById(definitionId);
    if (definition == nullptr || definition->pins.size() != component.pins.size())
    {
        return;
    }
    for (std::size_t index = 0; index < component.pins.size(); ++index)
    {
        component.pins[index].localPos = {definition->pins[index].localPosition.x,
                                          definition->pins[index].localPosition.y};
    }
}

void BackendAdapter::applyEditableValue(Component& component,
                                        int definitionId,
                                        const std::string& value) const
{
    switch (definitionId)
    {
        case 0:
            static_cast<Resistor&>(component).value = std::max(
                    resistanceValue(value, 1000.0), 1.0e-9);
            break;
        case 1:
            static_cast<Capacitor&>(component).value = std::max(
                    reactiveValue(value, 100.0e-9), 1.0e-15);
            break;
        case 2:
            static_cast<Battery&>(component).value = leadingNumber(value, 9.0);
            break;
        case 3:
        {
            const double frequency = std::max(leadingNumber(value, 1.0), 1.0e-9);
            static_cast<ClockGenerator&>(component).period = 1.0 / frequency;
            break;
        }
        case 4:
            static_cast<Switch&>(component).closed =
                    toUpperAscii(value).find("CLOSED") != std::string::npos;
            break;
        case 6:
            static_cast<LED&>(component).color = lowerAscii(trimAscii(value));
            break;
        case 8:
            static_cast<Inductor&>(component).value = std::max(
                    reactiveValue(value, 1.0e-3), 1.0e-15);
            break;
        case 9:
            static_cast<DCSource&>(component).value = leadingNumber(value, 5.0);
            break;
        case 10:
            static_cast<PushButton&>(component).pressed =
                    toUpperAscii(value).find("PRESSED") != std::string::npos &&
                    toUpperAscii(value).find("RELEASED") == std::string::npos;
            break;
        case 19:
            static_cast<Microcontroller&>(component).setInstructionPeriod(
                    std::max(timeValue(value, 1.0e-6), 1.0e-12));
            break;
        default:
            break;
    }
}
