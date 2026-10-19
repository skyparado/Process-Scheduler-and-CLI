#pragma once
// ============================================================================
// CommandInterpreter  -  CSOPESY Emulator
// Owner: Sky  (Command Interpreter + Config module)
//
// Reads a line of input, splits it into command + flag + argument, enforces the
// "initialize first" gate, recognizes the supported commands, dispatches to the
// Console / Scheduler, and prints the error messages for unknown commands,
// missing arguments, and "Process <name> not found.".
// ============================================================================
#include <string>
#include <iosfwd>

struct Config;
class Console;
class Scheduler;

class CommandInterpreter {
public:
    // All user-facing output (errors + acks) is written to `out`, which makes
    // the interpreter easy to unit-test with a std::ostringstream.
    CommandInterpreter(Config& cfg, Console& console, Scheduler& scheduler,
                       std::ostream& out);

    // A line split into its three parts. `flag` is empty when none was given
    // (a flag is a token beginning with '-'); `argument` is everything after
    // the command (and after the flag, if present), joined by single spaces.
    struct ParsedCommand {
        std::string command;
        std::string flag;
        std::string argument;
    };

    // Split a raw input line. Pure function -> directly unit-testable.
    static ParsedCommand parse(const std::string& line);

    // Handle one input line. Returns false when the app should exit.
    bool handle(const std::string& line);

    bool isInitialized() const { return initialized; }

private:
    Config&       cfg;
    Console&      console;
    Scheduler&    scheduler;
    std::ostream& out;
    bool          initialized = false;

    void doInitialize();
    void handleScreen(const ParsedCommand& pc);
};
