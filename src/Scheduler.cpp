// ============================================================================
// Scheduler.cpp  -  STUB (Scheduler teammate replaces the internals)
// ============================================================================
#include "Scheduler.h"
#include <ostream>
#include <algorithm>

Scheduler::Scheduler(const Config& cfg, std::ostream& out)
    : cfg(cfg), out(out) {}

void Scheduler::start() {
    if (running) {
        out << "scheduler-start: already running.\n";
        return;
    }
    running = true;
    // TODO (Scheduler teammate): begin generating a process every
    // cfg.batchProcessFreq CPU ticks (p01, p02, ...) and feeding the ready queue.
    out << "scheduler-start: generating dummy processes (batch-process-freq="
        << cfg.batchProcessFreq << ", scheduler=" << cfg.scheduler << ").\n";
}

void Scheduler::stop() {
    if (!running) {
        out << "scheduler-stop: scheduler is not running.\n";
        return;
    }
    running = false;
    out << "scheduler-stop: stopped generating dummy processes.\n";
}

bool Scheduler::createProcess(const std::string& name) {
    if (processes.count(name)) return false;
    Process process;
    process.name = name;
    process.pid = nextPid++;
    process.generateInstructions(cfg.minIns, cfg.maxIns);
    processes.emplace(name, process);
    return true;
}

bool Scheduler::hasProcess(const std::string& name) const {
    return processes.count(name) != 0;
}

const Process* Scheduler::getProcess(const std::string& name) const {
    const auto it = processes.find(name);
    return it == processes.end() ? nullptr : &it->second;
}

std::size_t Scheduler::processCount() const {
    return processes.size();
}

std::size_t Scheduler::finishedProcessCount() const {
    return static_cast<std::size_t>(std::count_if(
        processes.begin(), processes.end(),
        [](const auto& entry) { return entry.second.finished; }));
}

std::size_t Scheduler::runningProcessCount() const {
    return processCount() - finishedProcessCount();
}
