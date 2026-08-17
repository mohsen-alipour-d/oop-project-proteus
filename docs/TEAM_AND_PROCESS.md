# Team, Process & Workflow

## 1. Division of Labor

| # | Sub-project | Owner |
|---|---|---|
| 1 | Start menu | Melika |
| 2 | Design canvas (A4/A3/custom, zoom, pan) | Melika |
| 3 | Component library UI | Hana |
| 4 | Editing (drag, rotate, mirror, properties) | Hana |
| 5 | Wiring, junctions, nets | Mohsen |
| 6 | Base components (sources, passive, interactive) | Mohsen |
| 7 | Advanced library (MCU, ADC/DAC, LCD, Keypad, Memory) | Melika |
| 8 | Simulation control (Run/Pause/Step/Stop) | Hana |
| 9 | Measurement (probe, meters, oscilloscope) | Mohsen |
| 10 | File management, undo/redo, recent projects | Mohsen |
| 11 | DRC | Mohsen |

## 2. Git Workflow

- `main` is always green; features live on `feature/*` branches.
- Each feature: branch → edit → build → test → rebase on `origin/main` → push → PR.
- Only `CMakeLists.txt` registers new files; file names never change.
- Releases are tagged (`v1.0-backend`, `v1.0-final`).
- Build artifacts, `.idea/`, and runtime txt files are git-ignored.

## 3. Testing Strategy

- **Backend unit tests** (`tests/test_main.cpp`): pure domain, no UI.
- **Integration tests** (`tests/test_integration.cpp`): exercise
  `BackendAdapter` exactly as the UI does.
- A change lands only when both suites are green.

## 4. Milestones

1. Backend core + tests (sections 5, 6, 9, 10, 11) — tagged `v1.0-backend`.
2. Advanced library (section 7) with 69 dedicated checks.
3. Frontend (sections 1–4) and simulation control (8).
4. Full integration + mixed-signal analog solver — tagged `v1.0-final`.