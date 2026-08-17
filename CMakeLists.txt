cmake_minimum_required(VERSION 3.20)
project(ProteusOopIntegrated LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PROTEUS_BUILD_GUI "Build the SDL2 graphical application" ON)
option(PROTEUS_BUILD_TESTS "Build backend and integration tests" ON)

# Sections 5, 6, 7, 9, 10 and 11 plus the SDL-independent Section 8 engine.
set(PROTEUS_BACKEND_SOURCES
        src/core/pin.cpp
        src/core/component.cpp
        src/core/circuit.cpp
        src/wiring/wire.cpp
        src/wiring/junction.cpp
        src/wiring/net.cpp
        src/simulation/simulation_controller.cpp
        src/simulation/analog_solver.cpp
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
        src/components/advanced/adc.cpp
        src/components/advanced/dac.cpp
        src/components/advanced/microcontroller.cpp
        src/components/advanced/external_memory.cpp
        src/components/advanced/lcd16x2.cpp
        src/components/advanced/matrix_keypad.cpp
        src/mcu/register_file.cpp
        src/mcu/internal_ram.cpp
        src/mcu/flash_memory.cpp
        src/mcu/firmware_loader.cpp
        src/mcu/mcu_port.cpp
        src/mcu/instruction_decoder.cpp
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

add_library(proteus_backend STATIC ${PROTEUS_BACKEND_SOURCES})
target_include_directories(proteus_backend PUBLIC src)

# The integration target is SDL-independent, so it can be tested without a GUI.
add_library(proteus_integration STATIC
        src/ui/ComponentModel.cpp
        src/integration/backend_adapter.cpp
)
target_include_directories(proteus_integration PUBLIC src)
target_link_libraries(proteus_integration PUBLIC proteus_backend)

if (PROTEUS_BUILD_GUI)
    # Sections 1 to 4 and Section 8 controls: SDL2 frontend.
    find_package(SDL2 CONFIG QUIET)
    if (NOT TARGET SDL2::SDL2 AND NOT TARGET SDL2::SDL2-static)
        find_package(SDL2 REQUIRED)
    endif()

    find_package(SDL2_gfx CONFIG QUIET)

    add_executable(ProteusOopSimulator
            src/main.cpp
            src/ui/ProteusApplication.cpp
    )
    target_include_directories(ProteusOopSimulator PRIVATE src src/ui)
    target_link_libraries(ProteusOopSimulator PRIVATE proteus_integration)

    if (TARGET SDL2::SDL2main)
        target_link_libraries(ProteusOopSimulator PRIVATE SDL2::SDL2main)
    endif()
    if (TARGET SDL2::SDL2)
        target_link_libraries(ProteusOopSimulator PRIVATE SDL2::SDL2)
    elseif (TARGET SDL2::SDL2-static)
        target_link_libraries(ProteusOopSimulator PRIVATE SDL2::SDL2-static)
    else()
        target_include_directories(ProteusOopSimulator PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(ProteusOopSimulator PRIVATE ${SDL2_LIBRARIES})
    endif()

    if (TARGET SDL2_gfx::SDL2_gfx)
        target_link_libraries(ProteusOopSimulator PRIVATE SDL2_gfx::SDL2_gfx)
    elseif (TARGET SDL2::SDL2_gfx)
        target_link_libraries(ProteusOopSimulator PRIVATE SDL2::SDL2_gfx)
    elseif (TARGET unofficial::sdl2-gfx::sdl2-gfx)
        target_link_libraries(ProteusOopSimulator PRIVATE unofficial::sdl2-gfx::sdl2-gfx)
    else()
        find_path(SDL2_GFX_INCLUDE_DIR
                NAMES SDL2/SDL2_gfxPrimitives.h SDL2_gfxPrimitives.h)
        find_library(SDL2_GFX_LIBRARY NAMES SDL2_gfx SDL2_gfxd)
        if (NOT SDL2_GFX_INCLUDE_DIR OR NOT SDL2_GFX_LIBRARY)
            message(FATAL_ERROR
                    "SDL2_gfx was not found. Install sdl2-gfx with vcpkg and reload CMake.")
        endif()
        target_include_directories(ProteusOopSimulator PRIVATE ${SDL2_GFX_INCLUDE_DIR})
        target_link_libraries(ProteusOopSimulator PRIVATE ${SDL2_GFX_LIBRARY})
    endif()
endif()

if (PROTEUS_BUILD_TESTS)
    enable_testing()

    add_executable(run_tests tests/test_main.cpp)
    target_link_libraries(run_tests PRIVATE proteus_backend)
    add_test(NAME backend_tests COMMAND run_tests)

    add_executable(run_integration_tests tests/test_integration.cpp)
    target_link_libraries(run_integration_tests PRIVATE proteus_integration)
    add_test(NAME integration_tests COMMAND run_integration_tests)
endif()

if (MSVC)
    target_compile_options(proteus_backend PRIVATE /W4)
    target_compile_options(proteus_integration PRIVATE /W4)
    if (TARGET ProteusOopSimulator)
        target_compile_options(ProteusOopSimulator PRIVATE /W4)
    endif()
else()
    target_compile_options(proteus_backend PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(proteus_integration PRIVATE -Wall -Wextra -Wpedantic)
    if (TARGET ProteusOopSimulator)
        target_compile_options(ProteusOopSimulator PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
