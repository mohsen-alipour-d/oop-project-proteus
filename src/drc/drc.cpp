#include "drc.h"

#include "../components/interactive/led.h"
#include "../components/interactive/switch_comp.h"
#include "../components/passive/capacitor.h"
#include "../components/passive/inductor.h"
#include "../components/passive/resistor.h"
#include "../components/sources/battery.h"
#include "../components/sources/clock_gen.h"
#include "../components/sources/dc_source.h"
#include "../components/sources/ground.h"
#include "../components/digital/logic_level.h"
#include "../measurement/ammeter.h"
#include "../measurement/voltmeter.h"

#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

bool DRC::checkShorts(Circuit& c) {
    bool clean = true;
    for (Net* n : c.nets) {
        bool hasHigh = false;
        bool hasLow = false;
        string highName = "";
        string lowName = "";

        for (Pin* p : n->pins) {
            if (!p->drivesNet())
                continue;
            LogicLevel lvl = voltageToLogic(p->voltage);
            if (lvl == HIGH) {
                hasHigh = true;
                highName = p->owner->name;
            } else if (lvl == LOW) {
                hasLow = true;
                lowName = p->owner->name;
            }
        }

        if (hasHigh && hasLow) {
            string msg = "Short circuit: " + highName + " (HIGH) connected to " + lowName + " (LOW)";
            log.error(msg);
            clean = false;
        }
    }

    auto netOf = [&c](Pin* pin) -> Net* {
        for (Net* net : c.nets)
            if (net->hasPin(pin))
                return net;
        return nullptr;
    };
    for (Component* component : c.components) {
        double sourceVoltage = 0.0;
        bool isTwoTerminalSource = false;
        if (Battery* battery = dynamic_cast<Battery*>(component)) {
            sourceVoltage = battery->value;
            isTwoTerminalSource = true;
        } else if (DCSource* source = dynamic_cast<DCSource*>(component)) {
            sourceVoltage = source->value;
            isTwoTerminalSource = true;
        } else if (ClockGenerator* clock = dynamic_cast<ClockGenerator*>(component)) {
            sourceVoltage = clock->highVoltage;
            isTwoTerminalSource = true;
        }
        if (!isTwoTerminalSource || component->pins.size() < 2)
            continue;
        Net* first = netOf(&component->pins[0]);
        Net* second = netOf(&component->pins[1]);
        if (first != nullptr && first == second && std::fabs(sourceVoltage) > 1.0e-9) {
            log.error("Voltage source terminals are shorted on " + component->name);
            clean = false;
        }
    }
    return clean;
}

bool DRC::checkFloating(Circuit& c) {
    bool clean = true;
    for (Component* comp : c.components) {
        for (Pin& p : comp->pins) {
            if (!p.needsInputConnection())
                continue;
            if (!p.connected) {
                string msg = "Unconnected pin " + std::to_string(p.id + 1) +
                             " on " + comp->name;
                log.error(msg);
                clean = false;
            }
        }
    }
    return clean;
}

bool DRC::checkReferences(Circuit& c) {
    const int count = static_cast<int>(c.nets.size());
    if (count == 0)
        return true;

    std::vector<int> parent(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index)
        parent[static_cast<std::size_t>(index)] = index;

    auto find = [&parent](int value) {
        int current = value;
        while (parent[static_cast<std::size_t>(current)] != current) {
            parent[static_cast<std::size_t>(current)] =
                    parent[static_cast<std::size_t>(
                            parent[static_cast<std::size_t>(current)])];
            current = parent[static_cast<std::size_t>(current)];
        }
        return current;
    };
    auto unite = [&parent, &find](int first, int second) {
        if (first < 0 || second < 0)
            return;
        const int a = find(first);
        const int b = find(second);
        if (a != b)
            parent[static_cast<std::size_t>(b)] = a;
    };

    std::unordered_map<Pin*, int> pinNet;
    for (int index = 0; index < count; ++index)
        for (Pin* pin : c.nets[static_cast<std::size_t>(index)]->pins)
            pinNet[pin] = index;
    auto nodeOf = [&pinNet](Pin* pin) {
        const auto found = pinNet.find(pin);
        return found == pinNet.end() ? -1 : found->second;
    };

    for (Component* component : c.components) {
        const bool passiveBridge =
                dynamic_cast<Resistor*>(component) != nullptr ||
                dynamic_cast<Capacitor*>(component) != nullptr ||
                dynamic_cast<Inductor*>(component) != nullptr ||
                dynamic_cast<LED*>(component) != nullptr ||
                dynamic_cast<Ammeter*>(component) != nullptr ||
                dynamic_cast<Voltmeter*>(component) != nullptr ||
                dynamic_cast<Battery*>(component) != nullptr ||
                dynamic_cast<DCSource*>(component) != nullptr ||
                dynamic_cast<ClockGenerator*>(component) != nullptr ||
                (dynamic_cast<Switch*>(component) != nullptr &&
                 static_cast<Switch*>(component)->closed);
        if (passiveBridge && component->pins.size() >= 2)
            unite(nodeOf(&component->pins[0]), nodeOf(&component->pins[1]));
    }

    std::unordered_set<int> anchoredRoots;
    for (Component* component : c.components) {
        const bool relativeSource =
                dynamic_cast<Battery*>(component) != nullptr ||
                dynamic_cast<DCSource*>(component) != nullptr ||
                dynamic_cast<ClockGenerator*>(component) != nullptr;
        for (Pin& pin : component->pins) {
            const int node = nodeOf(&pin);
            if (node < 0)
                continue;
            if (dynamic_cast<Ground*>(component) != nullptr ||
                (pin.drivesNet() && !relativeSource))
                anchoredRoots.insert(find(node));
        }
    }

    bool clean = true;
    std::unordered_set<int> reported;
    for (int node = 0; node < count; ++node) {
        const int root = find(node);
        if (anchoredRoots.count(root) != 0 || reported.count(root) != 0)
            continue;
        reported.insert(root);
        const Net* net = c.nets[static_cast<std::size_t>(node)];
        const std::string nearName = !net->pins.empty() && net->pins.front()->owner
                ? net->pins.front()->owner->name
                : std::string("unknown component");
        log.error("Unreferenced electrical island near " + nearName +
                  "; connect its source/reference return to GND");
        clean = false;
    }
    return clean;
}

bool DRC::validate(Circuit& c) {
    log.clear();
    bool ok = true;
    if (!c.hasGround()) {
        log.error("Circuit has no ground reference");
        ok = false;
    }
    if (!checkShorts(c))
        ok = false;
    if (!checkFloating(c))
        ok = false;
    if (!checkReferences(c))
        ok = false;
    if (ok)
        log.info("Circuit passed DRC validation");
    return ok;
}
