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
    if (cfg.alternatingPrintAdd)
        out << "Alternating PRINT/ADD programs are on (alternating-print-add 1).\n";
    if (cfg.batchUnitTicks != 1)
        out << "Process generation is scaled: one batch-process-freq unit = "
            << cfg.batchUnitTicks << " ticks (batch-unit-ticks).\n";
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
        // Spec: a finished process's screen can no longer be re-attached.
        Process snapshot;
        if (!scheduler.getProcessSnapshot(pc.argument, snapshot) || snapshot.finished) {
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

    // --- Initialized: full command set ---
    if (pc.command == "initialize") {
        out << "Already initialized.\n";
        return true;
    }
    // Inside a process screen only 'process-smi' and 'exit' exist (spec).
    if (console.isInProcessScreen()) {
        if (pc.command == "exit") {
            console.leaveProcessScreen();
        } else if (pc.command == "process-smi") {
            console.printProcessSmi("");
        } else {
            out << "Unknown command: '" << pc.command << "'.\n";
        }
        return true;
    }
    if (pc.command == "exit") {
        return false;
    }
    if (pc.command == "screen") {
        handleScreen(pc);
        return true;
    }
    if (pc.command == "screen-ls") {   // the spec and quiz also spell it this way
        console.printScreenLs();
        return true;
    }
    if (pc.command == "scheduler-start" || pc.command == "scheduler-test") {   // spec's former name
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
    out << "Unknown command: '" << pc.command << "'.\n";
    return true;
}
