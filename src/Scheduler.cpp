
#include "Scheduler.h"
#include "Console.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

static const int TICK_INTERVAL_MS = 100; // ms per cpu cycle

Scheduler::Scheduler(const Config& cfg, std::ostream& out)
    : cfg(cfg), out(out) {
}

Scheduler::~Scheduler() {
    shutdown();
}

void Scheduler::initialize() {
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);
        if (initialized) {return;}

        shutdownRequested = false;
        generatorEnabled = false;

        cpuTicks = 0;
        ticksSinceGeneration = 0;

        // create  number of cpu cores
        cores.clear();

        for (int i = 0; i < cfg.numCpu; i++) {
            Core core;
            core.id = i;
            core.processName = "";
            core.quantumUsed = 0;

            cores.push_back(core);
        }
        initialized = true;
    }

    //start background cpu thread
    cpuThread = std::thread(&Scheduler::cpuLoop, this);
    logMessage("Scheduler initialized.");
}

//auto process generation
void Scheduler::start() {
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (!initialized || shutdownRequested) {return;}
        if (generatorEnabled) {return;}

        generatorEnabled = true;
        ticksSinceGeneration = 0;
    }
    logMessage("Scheduler started.");
}


//disable auto process generation
void Scheduler::stop() {
    bool wasRunning = false;

    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (generatorEnabled) {
            generatorEnabled = false;
            wasRunning = true;
        }
    }

    if (wasRunning) {
        logMessage("Scheduler stopped. Existing processes will continue running.");
    }
}

// stop cpu thread and waits for it to finish
void Scheduler::shutdown() {
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (!initialized) {return;}

        shutdownRequested = true;
        generatorEnabled = false;
    }

    if (cpuThread.joinable()) {cpuThread.join();}

    {
        std::lock_guard<std::mutex> lock(schedulerMutex);
        initialized = false;
    }
}


//check if scheduler is running
bool Scheduler::isRunning() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    return generatorEnabled;
}

//check if scheduler is initialized
bool Scheduler::isInitialized() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    return initialized;
}


bool Scheduler::createProcess(const std::string& name) {
    {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        if (!initialized || shutdownRequested) {return false;}
        if (processes.find(name) != processes.end()) {return false;}

        addProcess(name);
    }

    logMessage("Process created: " + name);
    return true;
}

//spec: a finished process can no longer be attached to with screen -r
bool Scheduler::canAttach(const std::string& name) const {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    auto it = processes.find(name);
    return it != processes.end() && !it->second.process.finished;
}

//copy process state for console
bool Scheduler::getProcessSnapshot(const std::string& name,Process& result) const {
    
    std::lock_guard<std::mutex> lock(schedulerMutex);

    auto it = processes.find(name);
    if (it == processes.end()) {
        return false;
    }

    result = it->second.process;
    return true;
}

//rows for screen -ls and report-util
std::vector<ProcessSummary> Scheduler::processSummaries() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    std::map<std::string, int> coreOf;
    for (const Core& core : cores) {
        if (!core.processName.empty()) {
            coreOf[core.processName] = core.id;
        }
    }

    std::vector<ProcessSummary> summaries;
    for (const auto& entry : processes) {
        const Process& process = entry.second.process;

        ProcessSummary summary;
        summary.name = process.name;
        summary.createdAt = process.createdAt;
        summary.pid = process.pid;
        summary.currentLine = process.currentLine;
        summary.totalLines = process.totalLines;
        summary.finished = process.finished;

        auto core = coreOf.find(process.name);
        if (core != coreOf.end()) {
            summary.core = core->second;
        }
        summaries.push_back(summary);
    }

    //the map is in name order, which would put p100 before p11
    std::sort(summaries.begin(), summaries.end(),
        [](const ProcessSummary& a, const ProcessSummary& b) { return a.pid < b.pid; });
    return summaries;
}

//core count
std::size_t Scheduler::coreCount() const {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    return cores.size();
}

void Scheduler::cpuLoop() {
    while (true) {
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);
            if (shutdownRequested) {break;}
        }
        runOneCpuCycle();
        std::this_thread::sleep_for(
            std::chrono::milliseconds(TICK_INTERVAL_MS)
        );
    }
}

//complete one cpu cycle
void Scheduler::runOneCpuCycle() {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    if (shutdownRequested) {
        return;
    }
    cpuTicks++;

    std::vector<std::string> sleepingAtStart;
    for (const auto& entry : processes) {
        if (entry.second.state == ProcessState::SLEEPING) {
            sleepingAtStart.push_back(entry.first);
        }
    }

    //assign ready processes to cores
    assignReadyProcesses();

    //execute each core
    for (Core& core : cores) {
        if (!core.processName.empty()) {
            executeCore(core);
        }
    }

    //advance sleep timer for sleeping processes
    for (const std::string& name : sleepingAtStart) {
        auto it = processes.find(name);
        if (it == processes.end()) {
            continue;
        }

        ManagedProcess& managed = it->second;
        if (managed.state != ProcessState::SLEEPING) {
            continue;
        }

        managed.process.tickSleep();

        if (managed.process.sleepTicks == 0 && !managed.process.finished) {
            managed.state = ProcessState::READY;
            readyQueue.push_back(name);
        }

        if (managed.process.finished) {
            managed.state = ProcessState::FINISHED;
        }
    }

    //generate new process
    if (generatorEnabled) {
        ticksSinceGeneration++;

        if (ticksSinceGeneration >= cfg.batchProcessFreq) {
            ticksSinceGeneration = 0;
            generateAutomaticProcess();
        }
    }

    //refill cores freed this cycle, or screen -ls reads them as idle between cycles
    assignReadyProcesses();
}

//assign ready processes to cores
void Scheduler::assignReadyProcesses() {
    for (Core& core : cores) {
        if (!core.processName.empty()) {
            continue;
        }

        while (!readyQueue.empty()) {
            std::string name = readyQueue.front();
            readyQueue.pop_front();

            auto it = processes.find(name);
            if (it == processes.end()) {
                continue;
            }

            ManagedProcess& managed = it->second;
            if (managed.state != ProcessState::READY) {continue;}
            managed.state = ProcessState::RUNNING;

            core.processName = name;
            core.quantumUsed = 0;
            break;
        }
    }
}

// execute one cycle for a core
void Scheduler::executeCore(Core& core) {
    auto it = processes.find(core.processName);

    if (it == processes.end()) {
        releaseCore(core);
        return;
    }

    ManagedProcess& managed = it->second;
    Process& process = managed.process;

    if (managed.state != ProcessState::RUNNING) {
        releaseCore(core);
        return;
    }

    //cpu cycle takes one quantum cycle
    core.quantumUsed++;

    //process waits before next instruction (busy-wait)
    if (managed.delayRemaining > 0) {
        managed.delayRemaining--;
    }
    else {
        std::size_t previousLine = process.currentLine;
        process.executeNext(core.id);

        if (process.currentLine > previousLine) {
            managed.delayRemaining = cfg.delaysPerExec;
        }
    }

    //process is done
    if (process.finished) {
        managed.state = ProcessState::FINISHED;
        releaseCore(core);
        return;
    }

    //sleep instruction releases the core
    if (process.sleepTicks > 0) {
        managed.state = ProcessState::SLEEPING;
        releaseCore(core);
        return;
    }

    //round robin
    if (cfg.scheduler == "rr" &&
        core.quantumUsed >= cfg.quantumCycles) {

        managed.state = ProcessState::READY;
        readyQueue.push_back(process.name);

        releaseCore(core);
    }
}

void Scheduler::releaseCore(Core& core) {
    core.processName.clear();
    core.quantumUsed = 0;
}


void Scheduler::generateAutomaticProcess() {
    std::ostringstream nameStream;

    nameStream << "p"
        << std::setw(2)
        << std::setfill('0')
        << nextGeneratedProcess++;

    std::string name = nameStream.str();

    //avoid overwriting an existing process
    if (processes.find(name) != processes.end()) {
        return;
    }

    addProcess(name);
}

void Scheduler::addProcess(const std::string& name) {
    ManagedProcess managed;

    managed.process.name = name;
    managed.process.pid = nextPid++;
    managed.process.createdAt = timestamp();
    managed.process.generateInstructions(cfg.minIns,cfg.maxIns);

    managed.state = ProcessState::READY;
    managed.delayRemaining = 0;

    processes.emplace(name, managed);
    readyQueue.push_back(name);
}

//log scheduler messages. Only call from the input thread: printing from the cpu
//thread would write over the prompt and into attached process screens
void Scheduler::logMessage(const std::string& message) {
    out << message << std::endl;
}
