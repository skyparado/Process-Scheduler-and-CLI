#pragma once
// ============================================================================
// Scheduler  -  STUB
// Owner (to implement fully): the "Scheduler + Main/Controller" teammate.
//
// Sky's Command Interpreter dispatches to this interface. The stub keeps just
// enough state (a set of known process names + a running flag) so that
// screen -s / screen -r / scheduler-start / scheduler-stop can be demonstrated
// end-to-end. The real tick loop, CPU cores, ready queue, FCFS/RR, delays, and
// SLEEP handling are added here by the Scheduler teammate.
// ============================================================================
#include "Config.h"
#include <string>
#include <set>
#include <iosfwd>

class Scheduler {
public:
    Scheduler(const Config& cfg, std::ostream& out);

    void start();                                   // scheduler-start
    void stop();                                    // scheduler-stop
    bool isRunning() const { return running; }

    // Returns false if a process with that name already exists.
    bool createProcess(const std::string& name);
    bool hasProcess(const std::string& name) const;

private:
    const Config&          cfg;
    std::ostream&          out;
    std::set<std::string>  processes;   // stub registry of created processes
    bool                   running = false;
    int                    nextPid = 1;
};
