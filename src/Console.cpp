#include "Console.h"
#include "Scheduler.h"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <vector>

namespace {
const char* clearScreen = "\x1b[2J\x1b[H";
const char* lastUpdated = "11/04/2026";     // change when the program changes
}

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
        << "Welcome to CSOPESY Emulator!\n"
        << "\n"
        << "Developers:\n"
        << "      Garcia, Theodore Rodolfo \n"
        << "      Magbatoc, Ethan Daniel \n"
        << "      Parado, Sky Hannah \n"
        << "      Villorente, Khyle Raeka\n"
        << "\n"
        << "Last updated: " << lastUpdated << "\n\n"
        << "Type 'initialize' to begin or 'exit' to quit.\n"
        << "=======================================================\n";
}

void Console::enterProcessScreen(const std::string& name) {
    processScreen = true;
    currentProcess = name;
    out << clearScreen;
    printProcessSmi(name);
}

// Layout follows the spec's process-smi mockup.
void Console::printProcessSmi(const std::string& name) {
    const std::string processName = name.empty() ? currentProcess : name;
    Process process;

    // get copy of process
    if (scheduler == nullptr ||
        !scheduler->getProcessSnapshot(processName, process)) {

        out << "Process " << processName << " not found.\n";
        return;
    }

    out << "\nProcess name: " << process.name << "\n"
        << "ID: " << process.pid << "\n"
        << "Logs:\n";

    for (const std::string& log : process.logs) {
        out << log << "\n";
    }
    out << "\n";

    if (process.finished) {
        out << "Finished!\n\n";
    } else {
        out << "Current instruction line: " << process.currentLine << "\n"
            << "Lines of code: " << process.totalLines << "\n\n";
    }
}

std::string Console::screenLsText() const {
    const std::size_t cores = scheduler == nullptr ? 0 : scheduler->coreCount();
    const std::size_t used = scheduler == nullptr ? 0 : scheduler->activeCoreCount();
    const std::size_t utilization = cores == 0 ? 0 : used * 100 / cores;
    const std::vector<Scheduler::ProcessInfo> rows =
        scheduler == nullptr ? std::vector<Scheduler::ProcessInfo>() : scheduler->listProcesses();

    std::ostringstream result;
    result << "CPU utilization: " << utilization << "%\n"
           << "Cores used: " << used << "\n"
           << "Cores available: " << cores - used << "\n\n"
           << "-------------------------------------------------------\n"
           << "Running processes:\n";
    // Running = on a core now; queued and sleeping processes wait their turn.
    for (const auto& row : rows) {
        if (row.finished || row.core < 0) continue;
        result << std::left << std::setw(12) << row.name
               << "(" << row.created << ")   "
               << "Core: " << std::setw(6) << row.core
               << row.currentLine << " / " << row.totalLines << "\n";
    }
    result << "\nFinished processes:\n";
    for (const auto& row : rows) {
        if (!row.finished) continue;
        result << std::left << std::setw(12) << row.name
               << "(" << row.created << ")   "
               << std::setw(12) << "Finished"
               << row.currentLine << " / " << row.totalLines << "\n";
    }
    result << "-------------------------------------------------------\n";
    return result.str();
}

void Console::printScreenLs() {
    out << screenLsText();
}

void Console::saveReport() {
    const char* path = "csopesy-log.txt";
    std::ofstream report(path);
    if (!report) {
        out << "Error: could not write csopesy-log.txt.\n";
        return;
    }
    report << screenLsText();
    out << "Report generated at "
        << std::filesystem::absolute(path).generic_string() << "!\n";
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
