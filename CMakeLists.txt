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
)

# Unit tests (Sky's Command Interpreter + Config)
add_executable(csopesy_tests
    tests/test_main.cpp
    src/Config.cpp
    src/CommandInterpreter.cpp
    src/Console.cpp
    src/Scheduler.cpp
)

enable_testing()
add_test(NAME unit COMMAND csopesy_tests)
