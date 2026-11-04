// ============================================================================
// main.cpp  -  CSOPESY Emulator entry point
// Owner: Ethan  (Scheduler + Main/Controller module)
//
// Reads a line of input and hands it to the Command Interpreter until 'exit'.
// The CPU tick loop runs on the Scheduler's own thread (see Scheduler.cpp).
// ============================================================================
#include "Config.h"
#include "Console.h"
#include "Scheduler.h"
#include "CommandInterpreter.h"

#include <iostream>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    // Let a classic cmd.exe window understand the ANSI clear-screen code.
    HANDLE console_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(console_out, &mode))
        SetConsoleMode(console_out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    Config    cfg;                       // defaults until 'initialize' runs
    Console   console(std::cout);
    Scheduler scheduler(cfg, std::cout);
    CommandInterpreter interp(cfg, console, scheduler, std::cout);

    console.printMainMenu();

    std::string line;
    while (true) {
        std::cout << console.prompt();
        if (!std::getline(std::cin, line)) break;   // EOF
        if (!interp.handle(line)) break;            // 'exit'
    }
    scheduler.shutdown();
    return 0;
}
