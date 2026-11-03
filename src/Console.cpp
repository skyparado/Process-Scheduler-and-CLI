#include "Console.h"
#include "Scheduler.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace {
const char* clearScreen = "\x1b[2J\x1b[H";

std::string timestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    std::ostringstream result;
    result << std::put_time(&localTime, "%m/%d/%Y %I:%M:%S%p");
    return result.str();
}
}

Console::Console(std::ostream& out) : out(out) {}

void Console::setScheduler(const Scheduler* value) {
    scheduler = value;
}

void Console::printMainMenu() {
    processScreen = false;
    currentProcess.clear();
    out << clearScreen
        << "=======================================================\n"
        << "   ____ ____   ___  ____  _____ ____ __   __\n"
        << "  / ___/ ___| / _ \\|  _ \\| ____/ ___|\\ \\ / /\n"
        << " | |   \\___ \\| | | | |_) |  _| \\___ \\ \\ V /\n"
        << " | |___ ___) | |_| |  __/| |___ ___) | | |\n"
        << "  \\____|____/ \\___/|_|   |_____|____/  |_|\n"
        << "=======================================================\n"
        << "Welcome to Group 3's CSOPESY Process Scheduler\n"
        << "\n"
        << "Developers:\n"
        << "      Garcia, Theodore Rodolfo \n"
        << "      Magbatoc, Ethan Daniel \n"
        << "      Parado, Sky Hannah \n"
        << "      Villorente, Khyle Raeka\n"
        << "\n"
        << "Last updated: " << timestamp() << "\n\n"
        << "Type 'initialize' to begin or 'exit' to quit.\n"
        << "=======================================================\n";
}

void Console::enterProcessScreen(const std::string& name) {
    processScreen = true;
    currentProcess = name;
    out << clearScreen
        << "=======================================================\n"
        << "Process screen: " << name << "\n"
        << "=======================================================\n"
        << "Type 'process-smi' for process details or 'exit' to return.\n";
}

void Console::printProcessSmi(const std::string& name) {
    const std::string processName = name.empty() ? currentProcess : name;
    const Process* process = scheduler == nullptr ? nullptr
                                                   : scheduler->getProcess(processName);
    if (process == nullptr) {
        out << "Process " << processName << " not found.\n";
        return;
    }

    out << "-------------------------------------------------------\n"
        << "Process name: " << process->name << "\n"
        << "Process ID: " << process->pid << "\n"
        << "Created: " << timestamp() << "\n"
        << "Current instruction line: " << process->currentLine << "\n"
        << "Lines of code: " << process->totalLines << "\n"
        << "Logs:\n";
    for (const std::string& log : process->logs) out << "  " << log << "\n";
    if (process->finished) out << "Finished!\n";
    out << "-------------------------------------------------------\n";
}

std::string Console::screenLsText() const {
    const std::size_t total = scheduler == nullptr ? 0 : scheduler->processCount();
    const std::size_t finished = scheduler == nullptr ? 0 : scheduler->finishedProcessCount();
    const std::size_t running = scheduler == nullptr ? 0 : scheduler->runningProcessCount();
    const std::size_t cores = scheduler == nullptr ? 0 : scheduler->coreCount();
    const std::size_t used = std::min(running, cores);
    const std::size_t utilization = cores == 0 ? 0 : used * 100 / cores;
    std::ostringstream result;
    result << "CPU utilization: " << utilization << "%\n"
           << "Cores used: " << used << "\n"
           << "Cores available: " << cores - used << "\n"
           << "Running processes: " << running << "\n"
           << "Finished processes: " << finished << "\n"
           << "Total processes: " << total << "\n";
    return result.str();
}

void Console::printScreenLs() {
    out << screenLsReport();
}

void Console::saveReport() {
    std::ofstream report("csopesy-log.txt");
    if (!report) {
        out << "Error: could not write csopesy-log.txt.\n";
        return;
    }
    report << screenLsReport();
    out << "report-util: saved screen -ls output to csopesy-log.txt.\n";
}

std::string Console::screenLsReport() const {
    std::ostringstream result;
    result << "-------------------------------------------------------\n"
           << "screen -ls (" << timestamp() << ")\n"
           << screenLsText()
           << "-------------------------------------------------------\n";
    return result.str();
}

void Console::leaveProcessScreen() {
    printMainMenu();
}

bool Console::isInProcessScreen() const {
    return processScreen;
}

std::string Console::prompt() const {
    return processScreen ? "root:\\" + currentProcess + "\\> " : "root:\\> ";
}
