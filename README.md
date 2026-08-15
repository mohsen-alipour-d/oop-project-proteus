# oop-project-proteus

A Proteus-like schematic editor and circuit simulator, built in modern C++
as the object-oriented programming course project (Spring 2026).

The repository currently contains the complete backend: component library,
wiring engine, electrical nets, measurement tools, file management and
design rule checks. The graphical frontend and the simulation control are
being built on top of this backend by other team members.

## Features (backend)

- Component library
  - Sources: ground, DC source, battery, clock generator
  - Passive: resistor, capacitor, inductor
  - Interactive: switch, push button, LED, 7-segment display
  - Digital: AND, OR, NOT, XOR, NAND gates and D flip-flop,
    with propagation delay and undefined-level handling
- Wiring engine
  - automatic pin detection with a sensitivity radius
  - 90-degree orthogonal wire routing
  - junction dots for real electrical connections
  - wire dragging and whole-net deletion
- Electrical nets built with union-find, plus voltage propagation
- Measurement tools: voltage probe, voltmeter, ammeter, oscilloscope
- File management: save / save-as / load, recent projects, undo / redo
- Design rule check: short-circuit and floating-input detection with a log buffer

## Build

Requires CMake >= 3.26 and a C++23 compiler.

    cmake -S . -B build
    cmake --build build

Or open the folder in CLion and let it load the CMake project.

## Run

Two executables are produced:

- `oop_project_proteus` : small backend demo and integration check
- `run_tests`           : unit tests (21 checks)

  ./build/oop_project_proteus
  ./build/run_tests

## Project layout

    src/core         base classes: vector2d, pin, component, circuit
    src/wiring       wire, junction, net
    src/components   sources / passive / interactive / digital
    src/measurement  probe, voltmeter, ammeter, oscilloscope
    src/file         serialization, file manager, undo/redo history
    src/drc          design rule check and log buffer
    tests            unit tests
    docs             team documents

## Team

- Mohsen: backend (sections 5, 6, 9, 10, 11)
- Melika: menus, canvas, MCU library (sections 1, 2, 7)
- Hana: library editor, simulation control (sections 3, 4, 8)
- final integration of 10/11 with the UI: shared

## Git workflow

- `main` : stable integrated code
- `feature/backend-core` : backend development branch
- each member works on their own feature branch and merges into main
  after the part is tested; see `docs/team_git_guide.md`