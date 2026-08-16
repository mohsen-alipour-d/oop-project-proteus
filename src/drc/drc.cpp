#include "drc.h"

#include "../components/digital/logic_level.h"

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
    return clean;
}

bool DRC::checkFloating(Circuit& c) {
    bool clean = true;
    for (Component* comp : c.components) {
        for (Pin& p : comp->pins) {
            if (!p.needsInputConnection())
                continue;
            if (!p.connected) {
                string msg = "Floating input on " + comp->name;
                log.error(msg);
                clean = false;
            }
        }
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
    if (ok)
        log.info("Circuit passed DRC validation");
    return ok;
}
