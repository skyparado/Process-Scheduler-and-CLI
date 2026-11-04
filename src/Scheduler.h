#pragma once
// ============================================================================
// Scheduler  -  STUB
// Owner: Ethan
//
// Sky's Command Interpreter dispatches to this interface. The stub keeps just
// enough state (a set of known process names + a running flag) so that
// screen -s / screen -r / scheduler-start / scheduler-stop can be demonstrated
// end-to-end. The real tick loop, CPU cores, ready queue, FCFS/RR, delays, and
// SLEEP handling are added here by the Scheduler teammate.
// ============================================================================
#include "Config.h"
#include "Process.h"

#include <string>
#include <map>
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <cstdint>
#include <iosfwd>

// One process's row in screen -ls and report-util.
struct ProcessSummary {
    std::string name;
    std::string createdAt;
    int         pid = 0;
    int         core = -1;          // -1: not on a core (ready, sleeping, finished)
    std::size_t currentLine = 0;
    std::size_t totalLines = 0;
    bool        finished = false;
};

class Scheduler {
public:

    Scheduler(const Config& cfg, std::ostream& out);
    ~Scheduler();

    void initialize(); //initialize cpu cores
    void start();
    void stop();

    void shutdown();
    bool isRunning() const;
    bool isInitialized() const;

    //process checking
    bool createProcess(const std::string& name);
    bool canAttach(const std::string& name) const; //exists and has not finished

    //copy of process state
    bool getProcessSnapshot(const std::string& name,Process& result) const;

    //all processes in creation order, taken under one lock
    std::vector<ProcessSummary> processSummaries() const;

    //cpu counts
    std::size_t coreCount() const;


private:

    enum class ProcessState {READY, RUNNING, SLEEPING, FINISHED};

    struct Core {
        int id = 0;
        std::string processName;
        uint64_t quantumUsed = 0; //num of cpu cylces
    };

    struct ManagedProcess {
        Process process;
        ProcessState state = ProcessState::READY;
        uint64_t delayRemaining = 0; // busy-wait cycles
    };

    //config and output
    const Config& cfg;
    std::ostream& out;

    //Process Management
    std::map<std::string, ManagedProcess> processes;
    std::deque<std::string> readyQueue; //FIFO queue
    std::vector<Core> cores; //available cores

    //Thread Management
    std::thread cpuThread;
    mutable std::mutex schedulerMutex;

    bool initialized = false;
    bool shutdownRequested = false;
    bool generatorEnabled = false;

    //cycle tracking
    uint64_t cpuTicks = 0;
    uint64_t ticksSinceGeneration = 0;

    int nextPid = 1;
    int nextGeneratedProcess = 1;

    void cpuLoop();
    void runOneCpuCycle();
    void assignReadyProcesses();
    void executeCore(Core& core);
    void handleSleepingProcesses();
    void releaseCore(Core& core);
    void generateAutomaticProcess();
    void addProcess(const std::string& name); //caller holds schedulerMutex
    void logMessage(const std::string& message);
};
