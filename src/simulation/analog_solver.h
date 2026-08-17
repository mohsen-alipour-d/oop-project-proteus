#pragma once

class Circuit;
class Component;

// Small mixed-signal nodal solver used by Section 8. It resolves voltages on
// passive analog nets instead of requiring an Output pin on every wire.
class AnalogSolver
{
public:
    static bool ownsComponentStep(const Component& component);
    static void solve(Circuit& circuit,
                      double dt,
                      bool advanceDynamicState);
};
