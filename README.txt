CSOPESY Major Output: Process Scheduler and CLI
================================================

GitHub: https://github.com/skyparado/Process-Scheduler-and-CLI

Entry point
-----------
The main() function is in:  src/main.cpp

HOW TO BUILD AND RUN
--------------------

Option A — MSVC (Visual Studio Build Tools), Windows
    build.bat            : builds build\csopesy.exe and build\csopesy_tests.exe
    build.bat test       : builds, then runs the unit tests
    build\csopesy.exe    : run the emulator

    (If build.bat cannot find vcvars64.bat, edit the VCVARS path near the top.)

Option B — CMake (any platform / compiler)
    cmake -S . -B build
    cmake --build build
    ctest --test-dir build        : run the unit tests
    build/csopesy (or build/Debug/csopesy.exe on MSVC) : run the emulator

Option C — g++ / MinGW
    g++ -std=c++17 -Isrc src/main.cpp src/Config.cpp src/CommandInterpreter.cpp ^
        src/Console.cpp src/Scheduler.cpp -o csopesy
    (tests: swap src/main.cpp for tests/test_main.cpp, add -o csopesy_tests)

USAGE
-----
On start, type `initialize` first — every other command except `exit` is
blocked until the config.txt parameters are loaded. Supported commands:

    initialize            load config.txt (num-cpu, scheduler, quantum-cycles,
                          batch-process-freq, min-ins, max-ins, delays-per-exec)
    screen -s <name>      create a new process screen
    screen -r <name>      re-attach to an existing process ("not found" if gone)
    screen -ls            list CPU utilization + running/finished processes
    scheduler-start       begin generating dummy processes (p01, p02, ...)
    scheduler-stop        stop generating dummy processes
    report-util           save the screen -ls report to csopesy-log.txt
    process-smi           (inside a screen) print this process's info
    exit                  quit the console

config.txt
----------
Space-separated key/value file read by `initialize`. Out-of-range or missing
values fall back to the spec defaults (num-cpu 4, scheduler rr, quantum-cycles 5,
batch-process-freq 1, min-ins 1000, max-ins 2000, delays-per-exec 0).

MODULE OWNERSHIP
----------------
Sky  : Command Interpreter (src/CommandInterpreter.*) + Config (src/Config.*),
       plus tests/test_main.cpp.
Stubs for teammates (clearly marked TODO), to be replaced with the real code:
       src/Console.*    Console UI / Display
       src/Scheduler.*  Scheduler + Main/Controller (tick loop, cores, FCFS/RR)
       src/Process.h    Process + Instructions (PRINT/DECLARE/ADD/.../FOR)
