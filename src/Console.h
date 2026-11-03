#pragma once
// ============================================================================
// Console  -  STUB
// Owner (to implement fully): the "Console UI / Display" teammate.
//
// Sky's Command Interpreter calls these methods; the stub prints clearly
// labelled placeholders so the dispatch is visible during a demo. The real
// ASCII header, process screen, process-smi layout, screen -ls table, and the
// report-util file write are implemented here by the Console UI teammate.
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
    std::string screenLsReport() const;
};
