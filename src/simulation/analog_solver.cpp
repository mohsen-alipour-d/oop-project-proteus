#include "analog_solver.h"

#include "../components/interactive/led.h"
#include "../components/interactive/switch_comp.h"
#include "../components/passive/capacitor.h"
#include "../components/passive/inductor.h"
#include "../components/passive/resistor.h"
#include "../components/sources/battery.h"
#include "../components/sources/clock_gen.h"
#include "../components/sources/dc_source.h"
#include "../components/sources/ground.h"
#include "../core/circuit.h"
#include "../measurement/ammeter.h"
#include "../measurement/voltmeter.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
constexpr double MIN_VALUE = 1.0e-12;
constexpr double OUTPUT_CONDUCTANCE = 1.0e9;
constexpr double GROUND_CONDUCTANCE = 1.0e10;
constexpr double CLOSED_SWITCH_RESISTANCE = 1.0e-6;
constexpr double AMMETER_RESISTANCE = 1.0e-6;
constexpr double VOLTMETER_RESISTANCE = 1.0e9;
constexpr double LED_ON_RESISTANCE = 100.0;
constexpr double LED_OFF_RESISTANCE = 1.0e9;
constexpr double ZERO_TIME_CAP_CONDUCTANCE = 1.0e9;
constexpr double GMIN = 1.0e-12;

struct DisjointSet
{
    explicit DisjointSet(int size) : parent(static_cast<std::size_t>(size))
    {
        for (int index = 0; index < size; ++index)
            parent[static_cast<std::size_t>(index)] = index;
    }

    int find(int value)
    {
        int& p = parent[static_cast<std::size_t>(value)];
        if (p != value)
            p = find(p);
        return p;
    }

    void unite(int first, int second)
    {
        if (first < 0 || second < 0)
            return;
        const int a = find(first);
        const int b = find(second);
        if (a != b)
            parent[static_cast<std::size_t>(b)] = a;
    }

    std::vector<int> parent;
};

struct VoltageSourceStamp
{
    Component* owner = nullptr;
    int positiveNode = -1;
    int negativeNode = -1;
    double voltage = 0.0;
};

bool isFinite(double value)
{
    return std::isfinite(value);
}

bool solveLinearSystem(std::vector<std::vector<double>> matrix,
                       std::vector<double> rightHandSide,
                       std::vector<double>& solution)
{
    const int size = static_cast<int>(rightHandSide.size());
    solution.assign(static_cast<std::size_t>(size), 0.0);

    for (int column = 0; column < size; ++column)
    {
        int pivot = column;
        for (int row = column + 1; row < size; ++row)
        {
            if (std::fabs(matrix[static_cast<std::size_t>(row)]
                                  [static_cast<std::size_t>(column)]) >
                std::fabs(matrix[static_cast<std::size_t>(pivot)]
                                  [static_cast<std::size_t>(column)]))
            {
                pivot = row;
            }
        }

        if (std::fabs(matrix[static_cast<std::size_t>(pivot)]
                            [static_cast<std::size_t>(column)]) < 1.0e-18)
            return false;

        if (pivot != column)
        {
            std::swap(matrix[static_cast<std::size_t>(pivot)],
                      matrix[static_cast<std::size_t>(column)]);
            std::swap(rightHandSide[static_cast<std::size_t>(pivot)],
                      rightHandSide[static_cast<std::size_t>(column)]);
        }

        const double diagonal = matrix[static_cast<std::size_t>(column)]
                                      [static_cast<std::size_t>(column)];
        for (int row = column + 1; row < size; ++row)
        {
            const double factor = matrix[static_cast<std::size_t>(row)]
                                        [static_cast<std::size_t>(column)] /
                                  diagonal;
            if (std::fabs(factor) < 1.0e-30)
                continue;
            matrix[static_cast<std::size_t>(row)]
                  [static_cast<std::size_t>(column)] = 0.0;
            for (int inner = column + 1; inner < size; ++inner)
            {
                matrix[static_cast<std::size_t>(row)]
                      [static_cast<std::size_t>(inner)] -=
                        factor * matrix[static_cast<std::size_t>(column)]
                                       [static_cast<std::size_t>(inner)];
            }
            rightHandSide[static_cast<std::size_t>(row)] -=
                    factor * rightHandSide[static_cast<std::size_t>(column)];
        }
    }

    for (int row = size - 1; row >= 0; --row)
    {
        double value = rightHandSide[static_cast<std::size_t>(row)];
        for (int column = row + 1; column < size; ++column)
        {
            value -= matrix[static_cast<std::size_t>(row)]
                           [static_cast<std::size_t>(column)] *
                     solution[static_cast<std::size_t>(column)];
        }
        const double diagonal = matrix[static_cast<std::size_t>(row)]
                                      [static_cast<std::size_t>(row)];
        if (std::fabs(diagonal) < 1.0e-18)
            return false;
        solution[static_cast<std::size_t>(row)] = value / diagonal;
        if (!isFinite(solution[static_cast<std::size_t>(row)]))
            return false;
    }
    return true;
}
}

bool AnalogSolver::ownsComponentStep(const Component& component)
{
    return dynamic_cast<const Resistor*>(&component) != nullptr ||
           dynamic_cast<const Capacitor*>(&component) != nullptr ||
           dynamic_cast<const Inductor*>(&component) != nullptr ||
           dynamic_cast<const LED*>(&component) != nullptr ||
           dynamic_cast<const Switch*>(&component) != nullptr ||
           dynamic_cast<const Battery*>(&component) != nullptr ||
           dynamic_cast<const DCSource*>(&component) != nullptr ||
           dynamic_cast<const Ground*>(&component) != nullptr ||
           dynamic_cast<const Ammeter*>(&component) != nullptr ||
           dynamic_cast<const Voltmeter*>(&component) != nullptr;
}

void AnalogSolver::solve(Circuit& circuit,
                         double dt,
                         bool advanceDynamicState)
{
    // Never leak readings or currents from a previous valid frame into a
    // newly disconnected/singular circuit.
    for (Component* component : circuit.components)
    {
        if (Ammeter* ammeter = dynamic_cast<Ammeter*>(component))
            ammeter->reading = 0.0;
        if (Voltmeter* voltmeter = dynamic_cast<Voltmeter*>(component))
        {
            voltmeter->reading = 0.0;
            voltmeter->hasError = true;
        }
        if (LED* led = dynamic_cast<LED*>(component))
            led->lit = false;
    }

    const int nodeCount = static_cast<int>(circuit.nets.size());
    if (nodeCount == 0)
        return;

    std::unordered_map<Pin*, int> pinNode;
    for (int node = 0; node < nodeCount; ++node)
    {
        Net* net = circuit.nets[static_cast<std::size_t>(node)];
        net->driverConflict = false;
        for (Pin* pin : net->pins)
            pinNode[pin] = node;
    }

    const auto nodeOf = [&pinNode](Pin* pin)
    {
        const auto found = pinNode.find(pin);
        return found == pinNode.end() ? -1 : found->second;
    };

    DisjointSet islands(nodeCount);
    const auto uniteTwoPin = [&islands, &nodeOf](Component* component)
    {
        if (component->pins.size() >= 2)
            islands.unite(nodeOf(&component->pins[0]),
                          nodeOf(&component->pins[1]));
    };

    for (Component* component : circuit.components)
    {
        if (dynamic_cast<Resistor*>(component) != nullptr ||
            dynamic_cast<Capacitor*>(component) != nullptr ||
            dynamic_cast<Inductor*>(component) != nullptr ||
            dynamic_cast<LED*>(component) != nullptr ||
            dynamic_cast<Ammeter*>(component) != nullptr ||
            dynamic_cast<Voltmeter*>(component) != nullptr ||
            dynamic_cast<Battery*>(component) != nullptr ||
            dynamic_cast<DCSource*>(component) != nullptr ||
            dynamic_cast<ClockGenerator*>(component) != nullptr)
        {
            uniteTwoPin(component);
        }
        else if (Switch* sw = dynamic_cast<Switch*>(component); sw && sw->closed)
        {
            uniteTwoPin(component);
        }
    }

    std::vector<bool> hasDriver(static_cast<std::size_t>(nodeCount), false);
    std::vector<bool> driverConflict(static_cast<std::size_t>(nodeCount), false);
    std::vector<double> driverTarget(static_cast<std::size_t>(nodeCount), 0.0);
    std::vector<bool> anchorNode(static_cast<std::size_t>(nodeCount), false);

    for (Component* component : circuit.components)
    {
        const bool twoTerminalSource =
                dynamic_cast<Battery*>(component) != nullptr ||
                dynamic_cast<DCSource*>(component) != nullptr ||
                dynamic_cast<ClockGenerator*>(component) != nullptr;

        for (Pin& pin : component->pins)
        {
            const int node = nodeOf(&pin);
            if (node < 0)
                continue;

            if (dynamic_cast<Ground*>(component) != nullptr)
                anchorNode[static_cast<std::size_t>(node)] = true;

            if (!pin.drivesNet() || twoTerminalSource)
                continue;

            const double target = isFinite(pin.voltage)
                                  ? pin.voltage
                                  : 0.0;
            if (!hasDriver[static_cast<std::size_t>(node)])
            {
                hasDriver[static_cast<std::size_t>(node)] = true;
                driverTarget[static_cast<std::size_t>(node)] = target;
            }
            else if (std::fabs(driverTarget[static_cast<std::size_t>(node)] -
                              target) > 1.0e-6)
            {
                driverConflict[static_cast<std::size_t>(node)] = true;
            }
            anchorNode[static_cast<std::size_t>(node)] = true;
        }
    }

    std::vector<VoltageSourceStamp> voltageSources;
    for (Component* component : circuit.components)
    {
        int positive = -1;
        int negative = -1;
        double voltage = 0.0;
        bool ideal = false;

        if (Battery* battery = dynamic_cast<Battery*>(component))
        {
            negative = nodeOf(&battery->pins[0]);
            positive = nodeOf(&battery->pins[1]);
            voltage = battery->value;
            ideal = battery->internalResistance <= MIN_VALUE;
        }
        else if (DCSource* source = dynamic_cast<DCSource*>(component))
        {
            negative = nodeOf(&source->pins[0]);
            positive = nodeOf(&source->pins[1]);
            voltage = source->value;
            ideal = true;
        }
        else if (ClockGenerator* clock = dynamic_cast<ClockGenerator*>(component))
        {
            positive = nodeOf(&clock->pins[0]);
            negative = nodeOf(&clock->pins[1]);
            voltage = clock->pins[0].voltage;
            ideal = true;
        }

        if (ideal && positive >= 0 && negative >= 0 && positive != negative)
            voltageSources.push_back({component, positive, negative, voltage});
    }

    std::vector<bool> islandAnchored(static_cast<std::size_t>(nodeCount), false);
    for (int node = 0; node < nodeCount; ++node)
    {
        if (anchorNode[static_cast<std::size_t>(node)])
            islandAnchored[static_cast<std::size_t>(islands.find(node))] = true;
    }

    std::unordered_map<LED*, bool> ledOn;
    for (Component* component : circuit.components)
    {
        if (LED* led = dynamic_cast<LED*>(component))
        {
            const int a = nodeOf(&led->pins[0]);
            const int b = nodeOf(&led->pins[1]);
            const double differential = a >= 0 && b >= 0
                    ? circuit.nets[static_cast<std::size_t>(a)]->voltage -
                      circuit.nets[static_cast<std::size_t>(b)]->voltage
                    : 0.0;
            ledOn[led] = led->lit || differential > led->threshold;
        }
    }

    std::vector<double> solution;
    const int sourceCount = static_cast<int>(voltageSources.size());
    const int matrixSize = nodeCount + sourceCount;

    for (int nonlinearPass = 0; nonlinearPass < 4; ++nonlinearPass)
    {
        std::vector<std::vector<double>> matrix(
                static_cast<std::size_t>(matrixSize),
                std::vector<double>(static_cast<std::size_t>(matrixSize), 0.0));
        std::vector<double> rhs(static_cast<std::size_t>(matrixSize), 0.0);

        const auto stampConductance = [&matrix](int a, int b, double conductance)
        {
            if (a < 0 || b < 0 || conductance <= 0.0 || !isFinite(conductance))
                return;
            matrix[static_cast<std::size_t>(a)][static_cast<std::size_t>(a)] += conductance;
            matrix[static_cast<std::size_t>(b)][static_cast<std::size_t>(b)] += conductance;
            matrix[static_cast<std::size_t>(a)][static_cast<std::size_t>(b)] -= conductance;
            matrix[static_cast<std::size_t>(b)][static_cast<std::size_t>(a)] -= conductance;
        };

        const auto stampCurrent = [&rhs](int from, int to, double current)
        {
            if (from < 0 || to < 0 || !isFinite(current))
                return;
            rhs[static_cast<std::size_t>(from)] -= current;
            rhs[static_cast<std::size_t>(to)] += current;
        };

        for (int node = 0; node < nodeCount; ++node)
        {
            matrix[static_cast<std::size_t>(node)]
                  [static_cast<std::size_t>(node)] += GMIN;
            if (hasDriver[static_cast<std::size_t>(node)])
            {
                matrix[static_cast<std::size_t>(node)]
                      [static_cast<std::size_t>(node)] += OUTPUT_CONDUCTANCE;
                rhs[static_cast<std::size_t>(node)] +=
                        OUTPUT_CONDUCTANCE *
                        driverTarget[static_cast<std::size_t>(node)];
            }
        }

        for (Component* component : circuit.components)
        {
            if (Ground* ground = dynamic_cast<Ground*>(component))
            {
                const int node = nodeOf(&ground->pins[0]);
                if (node >= 0)
                    matrix[static_cast<std::size_t>(node)]
                          [static_cast<std::size_t>(node)] += GROUND_CONDUCTANCE;
            }
            else if (Resistor* resistor = dynamic_cast<Resistor*>(component))
            {
                stampConductance(nodeOf(&resistor->pins[0]),
                                 nodeOf(&resistor->pins[1]),
                                 1.0 / std::max(resistor->value, MIN_VALUE));
            }
            else if (Capacitor* capacitor = dynamic_cast<Capacitor*>(component))
            {
                const int a = nodeOf(&capacitor->pins[0]);
                const int b = nodeOf(&capacitor->pins[1]);
                if (a < 0 || b < 0)
                    continue;
                const double conductance = dt > 0.0
                        ? std::max(capacitor->value, MIN_VALUE) / dt
                        : ZERO_TIME_CAP_CONDUCTANCE;
                stampConductance(a, b, conductance);
                rhs[static_cast<std::size_t>(a)] += conductance * capacitor->lastVoltage;
                rhs[static_cast<std::size_t>(b)] -= conductance * capacitor->lastVoltage;
            }
            else if (Inductor* inductor = dynamic_cast<Inductor*>(component))
            {
                const int a = nodeOf(&inductor->pins[0]);
                const int b = nodeOf(&inductor->pins[1]);
                if (dt > 0.0)
                {
                    stampConductance(a, b,
                                     dt / std::max(inductor->value, MIN_VALUE));
                }
                stampCurrent(a, b, inductor->lastCurrent);
            }
            else if (LED* led = dynamic_cast<LED*>(component))
            {
                const int a = nodeOf(&led->pins[0]);
                const int b = nodeOf(&led->pins[1]);
                if (a < 0 || b < 0)
                    continue;
                const bool on = ledOn[led];
                const double conductance = 1.0 /
                        (on ? LED_ON_RESISTANCE : LED_OFF_RESISTANCE);
                stampConductance(a, b, conductance);
                if (on)
                {
                    rhs[static_cast<std::size_t>(a)] += conductance * led->threshold;
                    rhs[static_cast<std::size_t>(b)] -= conductance * led->threshold;
                }
            }
            else if (Switch* sw = dynamic_cast<Switch*>(component))
            {
                if (sw->closed)
                    stampConductance(nodeOf(&sw->pins[0]),
                                     nodeOf(&sw->pins[1]),
                                     1.0 / CLOSED_SWITCH_RESISTANCE);
            }
            else if (Ammeter* ammeter = dynamic_cast<Ammeter*>(component))
            {
                stampConductance(nodeOf(&ammeter->pins[0]),
                                 nodeOf(&ammeter->pins[1]),
                                 1.0 / AMMETER_RESISTANCE);
            }
            else if (Voltmeter* voltmeter = dynamic_cast<Voltmeter*>(component))
            {
                stampConductance(nodeOf(&voltmeter->pins[0]),
                                 nodeOf(&voltmeter->pins[1]),
                                 1.0 / VOLTMETER_RESISTANCE);
            }
            else if (Battery* battery = dynamic_cast<Battery*>(component))
            {
                if (battery->internalResistance > MIN_VALUE)
                {
                    const int negative = nodeOf(&battery->pins[0]);
                    const int positive = nodeOf(&battery->pins[1]);
                    const double conductance = 1.0 / battery->internalResistance;
                    stampConductance(positive, negative, conductance);
                    stampCurrent(negative, positive,
                                 battery->value * conductance);
                }
            }
        }

        for (int sourceIndex = 0; sourceIndex < sourceCount; ++sourceIndex)
        {
            const VoltageSourceStamp& source =
                    voltageSources[static_cast<std::size_t>(sourceIndex)];
            const int currentIndex = nodeCount + sourceIndex;
            matrix[static_cast<std::size_t>(source.positiveNode)]
                  [static_cast<std::size_t>(currentIndex)] += 1.0;
            matrix[static_cast<std::size_t>(source.negativeNode)]
                  [static_cast<std::size_t>(currentIndex)] -= 1.0;
            matrix[static_cast<std::size_t>(currentIndex)]
                  [static_cast<std::size_t>(source.positiveNode)] += 1.0;
            matrix[static_cast<std::size_t>(currentIndex)]
                  [static_cast<std::size_t>(source.negativeNode)] -= 1.0;
            rhs[static_cast<std::size_t>(currentIndex)] = source.voltage;
        }

        if (!solveLinearSystem(std::move(matrix), std::move(rhs), solution))
        {
            for (Net* net : circuit.nets)
            {
                net->voltage = UNDEFINED_VOLT;
                net->voltageResolved = false;
            }
            for (Component* component : circuit.components)
            {
                if (!AnalogSolver::ownsComponentStep(*component))
                    continue;
                for (Pin& pin : component->pins)
                    pin.current = 0.0;
            }
            return;
        }

        bool ledStateChanged = false;
        for (auto& entry : ledOn)
        {
            LED* led = entry.first;
            const int a = nodeOf(&led->pins[0]);
            const int b = nodeOf(&led->pins[1]);
            if (a < 0 || b < 0)
                continue;
            const double differential = solution[static_cast<std::size_t>(a)] -
                                        solution[static_cast<std::size_t>(b)];
            const bool next = entry.second
                    ? differential > led->threshold - 0.05
                    : differential > led->threshold;
            ledStateChanged = ledStateChanged || next != entry.second;
            entry.second = next;
        }
        if (!ledStateChanged)
            break;
    }

    for (int node = 0; node < nodeCount; ++node)
    {
        Net* net = circuit.nets[static_cast<std::size_t>(node)];
        const bool anchored =
                islandAnchored[static_cast<std::size_t>(islands.find(node))];
        const double voltage = solution[static_cast<std::size_t>(node)];
        net->driverConflict = driverConflict[static_cast<std::size_t>(node)];
        net->voltageResolved = anchored && isFinite(voltage);
        net->voltage = net->driverConflict
                ? UNDEFINED_VOLT
                : (net->voltageResolved ? voltage : UNDEFINED_VOLT);
        for (Pin* pin : net->pins)
            pin->voltage = net->voltage;
    }

    for (Component* component : circuit.components)
    {
        if (component->pins.size() < 2)
            continue;
        const int a = nodeOf(&component->pins[0]);
        const int b = nodeOf(&component->pins[1]);
        if (a < 0 || b < 0 ||
            !circuit.nets[static_cast<std::size_t>(a)]->voltageResolved ||
            !circuit.nets[static_cast<std::size_t>(b)]->voltageResolved)
        {
            component->pins[0].current = 0.0;
            component->pins[1].current = 0.0;
            continue;
        }

        const double voltage = component->pins[0].voltage -
                               component->pins[1].voltage;
        if (Resistor* resistor = dynamic_cast<Resistor*>(component))
        {
            const double current = voltage / std::max(resistor->value, MIN_VALUE);
            resistor->pins[0].current = current;
            resistor->pins[1].current = -current;
        }
        else if (Capacitor* capacitor = dynamic_cast<Capacitor*>(component))
        {
            if (advanceDynamicState && dt > 0.0)
            {
                const double current = capacitor->value *
                        (voltage - capacitor->lastVoltage) / dt;
                capacitor->pins[0].current = current;
                capacitor->pins[1].current = -current;
                capacitor->lastVoltage = voltage;
            }
        }
        else if (Inductor* inductor = dynamic_cast<Inductor*>(component))
        {
            if (advanceDynamicState && dt > 0.0)
                inductor->lastCurrent += voltage / std::max(inductor->value, MIN_VALUE) * dt;
            inductor->pins[0].current = inductor->lastCurrent;
            inductor->pins[1].current = -inductor->lastCurrent;
        }
        else if (LED* led = dynamic_cast<LED*>(component))
        {
            const double forwardVoltage = voltage;
            led->lit = forwardVoltage > led->threshold;
            const double current = led->lit
                    ? std::max(0.0, (forwardVoltage - led->threshold) /
                                    LED_ON_RESISTANCE)
                    : 0.0;
            led->pins[0].current = current;
            led->pins[1].current = -current;
        }
        else if (Switch* sw = dynamic_cast<Switch*>(component))
        {
            const double current = sw->closed
                    ? voltage / CLOSED_SWITCH_RESISTANCE
                    : 0.0;
            sw->pins[0].current = current;
            sw->pins[1].current = -current;
        }
        else if (Ammeter* ammeter = dynamic_cast<Ammeter*>(component))
        {
            ammeter->reading = voltage / AMMETER_RESISTANCE;
            ammeter->pins[0].current = ammeter->reading;
            ammeter->pins[1].current = -ammeter->reading;
        }
        else if (Voltmeter* voltmeter = dynamic_cast<Voltmeter*>(component))
        {
            voltmeter->hasError = false;
            voltmeter->reading = voltage;
        }
        else if (Battery* battery = dynamic_cast<Battery*>(component))
        {
            if (battery->internalResistance > MIN_VALUE)
            {
                const double terminalVoltage = battery->pins[1].voltage -
                                               battery->pins[0].voltage;
                const double currentFromPositive =
                        (terminalVoltage - battery->value) /
                        battery->internalResistance;
                battery->pins[1].current = currentFromPositive;
                battery->pins[0].current = -currentFromPositive;
            }
        }
    }

    for (int sourceIndex = 0; sourceIndex < sourceCount; ++sourceIndex)
    {
        VoltageSourceStamp& source = voltageSources[static_cast<std::size_t>(sourceIndex)];
        const double current = solution[static_cast<std::size_t>(nodeCount + sourceIndex)];
        if (source.owner->pins.size() >= 2)
        {
            if (dynamic_cast<ClockGenerator*>(source.owner) != nullptr)
            {
                source.owner->pins[0].current = current;
                source.owner->pins[1].current = -current;
            }
            else
            {
                source.owner->pins[1].current = current;
                source.owner->pins[0].current = -current;
            }
        }
    }
}
