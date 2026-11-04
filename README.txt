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
        src/Console.cpp src/Scheduler.cpp src/Process.cpp -o csopesy
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

Console display
---------------
The console prints the CSOPESY header, developer names, and a timestamped
main menu. `screen -s` and `screen -r` clear the display and enter a
process-specific prompt. From that prompt, `process-smi` displays the process
ID, logs, current instruction line, total lines of code, and `Finished!` when
the process is complete. `exit` from a process screen returns to the main
menu; `exit` from the main menu quits the program.

`screen -ls` displays CPU utilization, cores used and available, then lists
each running process (creation time, core, line / total) and each finished
process. `screen -r` on a finished process prints "Process <name> not found.". `report-util` writes the same report to
`csopesy-log.txt`. All display timestamps use `MM/DD/YYYY HH:MM:SSAM/PM`.

Process instructions
--------------------
Each process gets a random program of min-ins to max-ins lines. A line is one
executed instruction, so a FOR counts as its body times its repeats, and
`Current instruction line` ends exactly at `Lines of code`.

    PRINT(msg, var)            log msg (default "Hello world from <name>!") + var
    DECLARE(var, value)        create a uint16 variable
    ADD(var1, var2/v, var3/v)  var1 = var2/v + var3/v
    SUBTRACT(var1, ...)        var1 = var2/v - var3/v
    SLEEP(X)                   sleep for X (uint8) CPU ticks
    FOR([instructions], n)     repeat n times, nested up to 3 levels

Variables are clamped to 0..65535, and undeclared ones read as 0. They are kept
until the process finishes. PRINT only writes to the process's logs, which
`process-smi` displays.

config.txt
----------
Space-separated key/value file read by `initialize`. Out-of-range or missing
values fall back to the spec defaults (num-cpu 4, scheduler rr, quantum-cycles 5,
batch-process-freq 1, min-ins 1000, max-ins 2000, delays-per-exec 0).

MODULE OWNERSHIP
----------------
Sky  : Command Interpreter (src/CommandInterpreter.*) + Config (src/Config.*),
       plus tests/test_main.cpp.
Theo : Process + Instructions (src/Process.*).
Console UI / Display (src/Console.*) and Scheduler + Main/Controller
(src/Scheduler.*, src/main.cpp: tick loop, cores, FCFS/RR) by the other members.
