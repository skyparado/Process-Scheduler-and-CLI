// ============================================================================
// Scheduler.cpp  -  STUB (Scheduler teammate replaces the internals)
// ============================================================================
#include "Scheduler.h"
#include <ostream>

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
    processes.insert(name);
    (void)nextPid++;   // pid assignment is the Scheduler teammate's concern
    return true;
}

bool Scheduler::hasProcess(const std::string& name) const {
    return processes.count(name) != 0;
}
