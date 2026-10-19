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

class Console {
public:
    explicit Console(std::ostream& out);

    void printMainMenu();                              // CSOPESY header + prompt
    void enterProcessScreen(const std::string& name);  // screen -s / screen -r
    void printProcessSmi(const std::string& name);     // process-smi
    void printScreenLs();                              // screen -ls
    void saveReport();                                 // report-util -> csopesy-log.txt

private:
    std::ostream& out;
};
