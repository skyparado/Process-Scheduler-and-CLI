#pragma once
// ============================================================================
// Console  -  CSOPESY Emulator
// Owner: Khyle  (Console UI / Display module)
//
// Prints the main menu, process screen, process-smi, and screen -ls, and writes
// the report-util file. Also owns the timestamp format used for all output.
// ============================================================================
#include <string>
#include <iosfwd>

class Scheduler;

// Current time as MM/DD/YYYY HH:MM:SSAM/PM, the one format used for all output.
std::string timestamp();

class Console {
public:
    explicit Console(std::ostream& out);

    void setScheduler(const Scheduler* scheduler);
    void printMainMenu();                              // CSOPESY header + prompt
    void enterProcessScreen(const std::string& name);  // screen -s / screen -r
    void printProcessSmi(const std::string& name);     // process-smi
    void printScreenLs();                              // screen -ls
    void saveReport();                                 // report-util -> csopesy-log.txt
    void leaveProcessScreen();
    bool isInProcessScreen() const;
    std::string prompt() const;

private:
    std::ostream& out;
    const Scheduler* scheduler = nullptr;
    std::string currentProcess;
    bool processScreen = false;

    std::string screenLsText() const;
};
