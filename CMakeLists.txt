cmake_minimum_required(VERSION 3.26)
project(oop_project_proteus)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_SOURCES
        src/core/pin.cpp
        src/core/component.cpp
        src/core/circuit.cpp
        src/wiring/wire.cpp
        src/wiring/junction.cpp
        src/wiring/net.cpp
        src/components/sources/ground.cpp
        src/components/sources/dc_source.cpp
        src/components/sources/battery.cpp
        src/components/sources/clock_gen.cpp
        src/components/passive/resistor.cpp
        src/components/passive/capacitor.cpp
        src/components/passive/inductor.cpp
        src/components/interactive/switch_comp.cpp
        src/components/interactive/push_button.cpp
        src/components/interactive/led.cpp
        src/components/interactive/seven_segment.cpp
        src/components/digital/logic_level.cpp
        src/components/digital/logic_gate.cpp
        src/components/digital/logic_gates.cpp
        src/components/digital/d_flipflop.cpp
        src/measurement/voltage_probe.cpp
        src/measurement/voltmeter.cpp
        src/measurement/ammeter.cpp
        src/measurement/oscilloscope.cpp
        src/file/component_factory.cpp
        src/file/history.cpp
        src/file/file_manager.cpp
        src/drc/log_buffer.cpp
        src/drc/drc.cpp
)

add_executable(oop_project_proteus src/main.cpp ${CORE_SOURCES})
add_executable(run_tests tests/test_main.cpp ${CORE_SOURCES})