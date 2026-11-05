cmake_minimum_required(VERSION 3.15)
project(CSOPESY_Emulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main emulator
add_executable(csopesy
    src/main.cpp
    src/Config.cpp
    src/CommandInterpreter.cpp
    src/Console.cpp
    src/Scheduler.cpp
    src/Process.cpp
)

# Visual Studio runs the exe from its build folder, where 'initialize' looks for
# config.txt. This runs on every build, so an edited config.txt takes effect on Run.
add_custom_target(copy_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/config.txt ${CMAKE_BINARY_DIR}/config.txt
)
add_dependencies(csopesy copy_config)

# Unit tests (Command Interpreter + Config, Process + Instructions)
add_executable(csopesy_tests
    tests/test_main.cpp
    src/Config.cpp
    src/CommandInterpreter.cpp
    src/Console.cpp
    src/Scheduler.cpp
    src/Process.cpp
)

enable_testing()
add_test(NAME unit COMMAND csopesy_tests)
