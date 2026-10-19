// ============================================================================
// Console.cpp  -  STUB (Console UI teammate replaces the internals)
// ============================================================================
#include "Console.h"
#include <ostream>

Console::Console(std::ostream& out) : out(out) {}

void Console::printMainMenu() {
    // Placeholder banner. The Console UI teammate owns the final CSOPESY ASCII
    // art, developer names, last-updated date, and formatting.
    out << "-------------------------------------------------------\n";
    out << "   CSOPESY Emulator\n";
    out << "   Welcome to CSOPESY Emulator!\n";
    out << "-------------------------------------------------------\n";
    out << "Type 'initialize' to begin.\n";
}

void Console::enterProcessScreen(const std::string& name) {
    out << "[console] entering screen for process '" << name << "'.\n";
    // TODO (Console teammate): clear screen, show process view, handle
    // process-smi / exit inside the screen sub-loop.
}

void Console::printProcessSmi(const std::string& name) {
    out << "[console] process-smi for '" << name << "'.\n";
    // TODO (Console teammate): name, ID, logs, current instruction line,
    // lines of code, "Finished!" when done.
}

void Console::printScreenLs() {
    out << "[console] screen -ls: CPU utilization / cores used / cores available"
           " / running + finished processes.\n";
    // TODO (Console teammate): real utilization table.
}

void Console::saveReport() {
    out << "[console] report-util: saving screen -ls output to csopesy-log.txt.\n";
    // TODO (Console teammate): write the same info to csopesy-log.txt.
}
