# Architecture & Design

This document describes the essence, layers and key decisions of the project.

## 1. Layered Design

| Layer | Location | Responsibility |
|---|---|---|
| Presentation | `src/ui` | SDL2 rendering, input, dialogs, library panel |
| Integration | `src/integration` | `BackendAdapter`: the only bridge between UI and domain |
| Domain | `src/core`, `src/components`, `src/measurement` | Circuit model, component behavior |
| Services | `src/file`, `src/drc`, `src/simulation` | Serialization, validation, simulation engine |

Rule: upper layers call downward only. The UI holds `ComponentInstance`
value-objects and never raw domain pointers.

## 2. Domain Model & Ownership

- `Circuit` **owns** all `Component`, `Wire`, `Junction`, `Net` objects.
  Components are created with `new` and handed to `addComponent`; the circuit
  frees everything on `clear()` / destruction.
- `Component` is the polymorphic base; each type overrides `step()`,
  `typeTag()` and `extraData()` (used by serialization).
- `Pin` carries voltage, current, connection state and a **direction model**:
  `Input`, `Output`, `Bidirectional`. `drivesNet()` and
  `needsInputConnection()` unify legacy `isOutput` with the new model so
  dynamic pins (MCU, keypad, memory) behave correctly.
- `Net` groups electrically connected pins (via wires + junctions) using
  union-find; it holds the shared voltage.

> Crossing wires are **not** connected. Electrical connection only exists
> through an explicit `Junction`.

## 3. Simulation Engine

- `SimulationController` owns the Run/Pause/Stop/Step state machine and time.
- `BackendAdapter::evaluateCircuit(dt)`:
    1. steps all components (pass 0 uses real `dt`, later settle passes use 0),
    2. `propagateVoltages()` spreads digital drivers over nets,
    3. `AnalogSolver::solve()` applies MNA for sources, R/C/L, LED, switch,
       meters (Backward-Euler for energy storage),
    4. the oscilloscope samples solved net voltages.
- Digital logic is 3-valued with dominance (HIGH dominates in OR, LOW in AND),
  so partially-undefined inputs resolve sensibly.

## 4. Serialization & History

- Token-based, space-separated lines: `TAG name x y rot mH mV [extra...]`,
  then `W` (wire) and `J` (junction) lines.
- Deserialization is hardened: malformed lines are skipped, pin indices are
  bounds-checked, and failures never crash the loader.
- `History` stores snapshots (max 50) for undo/redo; every user mutation
  records a snapshot.

## 5. Design Rule Check (DRC)

Before Run the circuit must pass: ground reference present, no logical short
(HIGH driver vs LOW driver on one net), no floating inputs, and no
unreferenced electrical island.

## 6. Key Decisions & Trade-offs

- **No SPICE-grade solver:** a simplified MNA gives stable, educational
  transients (RC/RL) at low complexity; node voltages on purely passive nets
  are solved, not left floating.
- **Single entry point** (`src/main.cpp`) and a single integration bridge keep
  merges conflict-free across three developers.
- **Tests before UI:** the domain is validated by 105 backend checks, then 50
  integration checks exercise the UI-facing adapter.