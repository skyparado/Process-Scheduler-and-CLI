// ============================================================================
// CommandInterpreter.cpp  -  Owner: Sky
// ============================================================================
#include "CommandInterpreter.h"
#include "Config.h"
#include "Console.h"
#include "Scheduler.h"

#include <ostream>
#include <sstream>
#include <vector>
#include <string>

CommandInterpreter::CommandInterpreter(Config& cfg, Console& console,
                                       Scheduler& scheduler, std::ostream& out)
    : cfg(cfg), console(console), scheduler(scheduler), out(out) {
    console.setScheduler(&scheduler);
}

// ----------------------------------------------------------------------------
// parse: command + flag + argument
// ----------------------------------------------------------------------------
CommandInterpreter::ParsedCommand
CommandInterpreter::parse(const std::string& line) {
    ParsedCommand pc;

    std::istringstream ls(line);
    std::vector<std::string> tokens;
    std::string tok;
    while (ls >> tok) tokens.push_back(tok);

    if (tokens.empty()) return pc;      // all three fields empty

    pc.command = tokens[0];

    size_t i = 1;
    if (i < tokens.size() && !tokens[i].empty() && tokens[i][0] == '-') {
        pc.flag = tokens[i];
        ++i;
    }

    // Join the remaining tokens (the argument) with single spaces.
    std::string arg;
    for (; i < tokens.size(); ++i) {
        if (!arg.empty()) arg += ' ';
        arg += tokens[i];
    }
    pc.argument = arg;

    return pc;
}

// ----------------------------------------------------------------------------
void CommandInterpreter::doInitialize() {
    cfg = Config::loadFromFile("config.txt", out);
    scheduler.initialize();
    initialized = true;
    out << "Initialized. (num-cpu=" << cfg.numCpu
        << ", scheduler=" << cfg.scheduler
        << ", quantum-cycles=" << cfg.quantumCycles
        << ", batch-process-freq=" << cfg.batchProcessFreq
        << ", min-ins=" << cfg.minIns
        << ", max-ins=" << cfg.maxIns
        << ", delays-per-exec=" << cfg.delaysPerExec << ")\n";
}

// ----------------------------------------------------------------------------
void CommandInterpreter::handleScreen(const ParsedCommand& pc) {
    if (pc.flag == "-s") {
        if (pc.argument.empty()) {
            out << "Error: 'screen -s' requires a process name.\n";
            return;
        }
        if (!scheduler.createProcess(pc.argument)) {
            out << "Process " << pc.argument << " already exists.\n";
            return;
        }
        console.enterProcessScreen(pc.argument);
    } else if (pc.flag == "-r") {
        if (pc.argument.empty()) {
            out << "Error: 'screen -r' requires a process name.\n";
            return;
        }
        if (!scheduler.canAttach(pc.argument)) {
            out << "Process " << pc.argument << " not found.\n";
            return;
        }
        console.enterProcessScreen(pc.argument);
    } else if (pc.flag == "-ls") {
        console.printScreenLs();
    } else {
        out << "Error: invalid 'screen' usage. Expected 'screen -s <name>', "
               "'screen -r <name>', or 'screen -ls'.\n";
    }
}

// ----------------------------------------------------------------------------
bool CommandInterpreter::handle(const std::string& line) {
    ParsedCommand pc = parse(line);

    if (pc.command.empty()) return true;   // blank line -> ignore

    // --- Gate: before initialize, only 'initialize' and 'exit' are allowed ---
    if (!initialized) {
        if (pc.command == "initialize") { doInitialize(); return true; }
        if (pc.command == "exit")       { return false; }
        out << "Please run 'initialize' first.\n";
        return true;
    }

    // --- Gate: a process screen only accepts 'process-smi' and 'exit' ---
    if (console.isInProcessScreen() && pc.command != "process-smi" && pc.command != "exit") {
        out << "'" << pc.command << "' is not available in a process screen. "
               "Type 'exit' to return to the main menu.\n";
        return true;
    }

    // --- Initialized: full command set ---
    if (pc.command == "initialize") {
        out << "Already initialized.\n";
        return true;
    }
    if (pc.command == "exit") {
        if (console.isInProcessScreen()) {
            console.leaveProcessScreen();
            return true;
        }
        return false;
    }
    if (pc.command == "screen") {
        handleScreen(pc);
        return true;
    }
    if (pc.command == "scheduler-start") {
        scheduler.start();
        return true;
    }
    if (pc.command == "scheduler-stop") {
        scheduler.stop();
        return true;
    }
    if (pc.command == "report-util") {
        console.saveReport();
        return true;
    }
    if (pc.command == "process-smi") {
        // Typed from within a process screen; the Console teammate resolves the
        // current screen. Argument (if any) names the process to inspect.
        console.printProcessSmi(pc.argument);
        return true;
    }

    out << "Unknown command: '" << pc.command << "'.\n";
    return true;
}
