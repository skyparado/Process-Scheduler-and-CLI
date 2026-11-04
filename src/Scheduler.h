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
    bool hasProcess(const std::string& name) const;

    //copy of process state
    bool getProcessSnapshot(const std::string& name,Process& result) const;

    //process counts
    std::size_t processCount() const;
    std::size_t finishedProcessCount() const;
    std::size_t runningProcessCount() const;

    //cpu counts
    std::size_t coreCount() const;
    std::size_t activeCoreCount() const;

    //one row of screen -ls / report-util
    struct ProcessInfo {
        std::string name;
        std::string created;
        int core = -1;              // -1: not on a core right now
        std::size_t currentLine = 0;
        std::size_t totalLines = 0;
        bool finished = false;
    };
    std::vector<ProcessInfo> listProcesses() const; // in creation order


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
        std::string created;         // timestamp() at creation
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
    std::mutex outputMutex;

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
    void addProcess(const std::string& name); // caller holds schedulerMutex
    void logMessage(const std::string& message);
};
