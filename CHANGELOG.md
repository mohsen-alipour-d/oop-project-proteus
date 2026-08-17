# Changelog

All notable changes to this project are documented here.
The format follows Keep a Changelog (https://keepachangelog.com/).

## [v1.0-final] - Final delivery
### Added
- SDL2 graphical frontend (menu, canvas, library, properties, oscilloscope).
- Mixed-signal analog solver (MNA) with Backward-Euler C/L models.
- Live interaction (switch / push button / value scroll) during Run and Pause.
- 155 automated checks (105 backend + 50 integration).
- GitHub Actions CI workflow.
### Fixed
- Battery/DC-source terminal polarity aligned with the UI.
- Passive-node voltages solved instead of being reported FLOATING.
- LED lit-state rendering.
- Hardened project deserialization against malformed lines.

## [v1.0-backend] - Backend milestone
### Added
- Wiring, junctions, nets (union-find), 3-valued digital logic.
- Base + advanced components (MCU, ADC/DAC, LCD, Keypad, Memory).
- Measurement (probe, meters, oscilloscope), DRC, file/undo/redo.
