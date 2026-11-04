// ============================================================================
// main.cpp  -  CSOPESY Emulator entry point (thin skeleton)
//
// This thin driver wires Sky's Command Interpreter + Config to the stub
// Console / Scheduler so the module runs and can be demoed. The Scheduler
// teammate owns the real main()/tick-loop; this version reads a line and hands
// it to the interpreter.
// ============================================================================
#include "Config.h"
#include "Console.h"
#include "Scheduler.h"
#include "CommandInterpreter.h"

#include <iostream>
#include <string>

int main() {
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
