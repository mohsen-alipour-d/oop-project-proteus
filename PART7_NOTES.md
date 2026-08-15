# Part 7 — Advanced Component Library

This project extension implements Part 7 of the Proteus OOP assignment while keeping the existing Parts 5, 6, 9, 10, and 11 architecture intact.

## Implemented requirements

### 7.1 ADC
- Configurable output width: 1–31 bits.
- Pins: `Vin`, `Vref+`, `Vref-`, then `D0..D(N-1)`.
- Linear mapping from `[Vref-, Vref+]` to `[0, 2^N-1]`.
- Saturation outside the reference range.
- Configurable conversion delay.
- Previous valid output is held while a conversion is pending.

### 7.2 DAC
- Configurable input width: 1–31 bits.
- Pins: `D0..D(N-1)`, `Vref+`, `Vref-`, `Vout`.
- Linear mapping from the digital code to the reference-voltage range.
- Configurable conversion delay with output hold during conversion.

### 7.3 HEX firmware loader
- `FirmwareLoader` reads Intel HEX files.
- Validates record shape, byte count, hexadecimal characters, checksum, and EOF record.
- Supports data records and extended segment/linear address records within the simulated Flash size.
- Loads bytes into `FlashMemory`.

### 7.4 Object-oriented MCU internals
The MCU is split into focused classes:
- `ProgramCounter`
- `RegisterFile` (Accumulator + R0..R7)
- `InternalRAM`
- `FlashMemory`
- `MCUPort`
- `InstructionDecoder`
- `Microcontroller` component that composes the above objects

### 7.5 Instruction decoder
The assignment names the required operations but does not define a concrete MCU/opcode format. The simulator therefore provides a small documented educational ISA and also recognizes a useful 8051-style subset for the same operations.

#### Educational ISA
All numbers below are hexadecimal bytes.

| Encoding | Meaning |
|---|---|
| `10 reg imm` | `MOV reg, #imm` |
| `11 dst src` | `MOV dst, src` |
| `12 reg port` | `MOV reg, PORT` |
| `13 port reg` | `MOV PORT, reg` |
| `20 reg imm` | `ADD reg, #imm` |
| `30 hi lo` | `JMP address16` |
| `40 port bit` | `SETB port.bit` |
| `50 port bit` | `CLR port.bit` |

Register `0` is the accumulator and registers `1..8` are `R0..R7`. Port `0` is Port A and port `1` is Port B.

#### Supported 8051-style subset
- `74 data` — `MOV A,#data`
- `78..7F data` — `MOV R0..R7,#data`
- `24 data` — `ADD A,#data`
- `28..2F` — `ADD A,R0..R7`
- `02 hi lo` — `LJMP address16`
- `D2 bit` — `SETB bit`
- `C2 bit` — `CLR bit`
- `E4` — `CLR A`
- `75 direct data` — `MOV direct,#data`
- `E5 direct` — `MOV A,direct`
- `F5 direct` — `MOV direct,A`

For the simplified direct-address mapping, `0x80` is Port A and `0x90` is Port B; other in-range direct addresses use internal RAM.

### 7.6 MCU I/O ports
- Two 8-bit ports: Port A and Port B.
- Each pin is bidirectional.
- Each port has an output-direction mask, output latch, pin sampling, bit set, and bit clear operations.
- Existing `Wire`/`Net` propagation is reused; no second wiring system was added.

### 7.7 External memory
- `ExternalMemory` models byte-addressable external RAM/EEPROM-style storage.
- Configurable address width from 1–16 bits.
- Pins: address bus, 8-bit bidirectional data bus, `RD`, `WR`.
- `RD` and `WR` are active high in this simplified simulator.
- Memory contents are included in project save/load serialization.

### 7.8 LCD 16x2
- 16 columns × 2 rows.
- 8-bit data interface plus `RS`, `RW`, `E`.
- Latches on the rising edge of `E`.
- Supports character writes, clear (`0x01`), home (`0x02`), and cursor/DDRAM-address commands (`0x80 + address`).
- `line1()` and `line2()` expose live text for the frontend to render.

### 7.9 Matrix keypad
- 4×4 keypad.
- Pins: `R0..R3`, `C0..C3`.
- Supports press/release operations and standard labels:
  - `1 2 3 A`
  - `4 5 6 B`
  - `7 8 9 C`
  - `* 0 # D`
- During scanning, a pressed key propagates the driven column level to its row, allowing an MCU input to detect it.

## Core compatibility changes

### Pin direction
`PinDirection` was added with:
- `Input`
- `Output`
- `Bidirectional`

The original `isOutput` field is still supported so all existing components remain compatible.

`Circuit::propagateVoltages()` and DRC now use `Pin::drivesNet()` / `Pin::needsInputConnection()` so dynamic MCU/memory/keypad pins work correctly.

### Multi-character component type tags
The component factory now reads the full type token instead of only its first character. Existing tags such as `R`, `C`, `L`, etc. continue to work, while Part 7 uses:
- `ADC`
- `DAC`
- `MCU`
- `MEM`
- `LCD`
- `KEYPAD`

## Save/load integration
Part 7 components are reconstructed by the existing `FileManager` / component factory.
- ADC/DAC configuration is preserved.
- MCU instruction period, port direction masks, and Flash contents are preserved.
- External-memory contents are preserved.
- LCD and keypad component types are preserved.

Runtime LCD/keypad state is intentionally not serialized because saving the full running simulation state is the separate bonus behavior in Part 10.

## Tests
`tests/test_main.cpp` now covers:
- Existing project behavior.
- Pin-direction backward compatibility.
- ADC range, saturation, bit width, delay, save/load.
- DAC mapping, bit width, delay, save/load.
- Intel HEX loading and checksum validation.
- MCU registers, RAM, PC, custom decoder, 8051-style subset, and ports.
- External memory read/write.
- LCD commands and character output.
- Keypad scanning.
- Save/load for all new Part 7 component types.
- Integration through the existing circuit wiring system:
  - MCU -> LCD
  - Keypad -> MCU
  - MCU <-> external memory

At completion the test executable reports **69 passed, 0 failed**.

## Build
From the project root:

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/run_tests
```

A different CMake generator can be used if Ninja is unavailable.
