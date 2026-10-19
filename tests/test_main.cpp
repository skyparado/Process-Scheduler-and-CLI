// ============================================================================
// test_main.cpp  -  unit tests for Sky's Command Interpreter + Config module
//
// Tiny assertion harness (no external framework). Builds as its own executable;
// does not need the teammates' real Console/Scheduler/Process code -- it uses
// the stubs, driving everything through injected string streams.
// ============================================================================
#include "../src/Config.h"
#include "../src/Console.h"
#include "../src/Scheduler.h"
#include "../src/CommandInterpreter.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdio>

static int g_failures = 0;
static int g_checks   = 0;

static void check(bool cond, const std::string& what) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::cout << "  FAIL: " << what << "\n";
    }
}

static bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

// Write a temp config file, return its path.
static std::string writeTemp(const std::string& name, const std::string& body) {
    std::ofstream f(name);
    f << body;
    f.close();
    return name;
}

// ---------------------------------------------------------------------------
static void test_parse() {
    std::cout << "[parse]\n";
    using CI = CommandInterpreter;

    auto a = CI::parse("screen -s p01");
    check(a.command == "screen" && a.flag == "-s" && a.argument == "p01",
          "'screen -s p01' splits into command/flag/argument");

    auto b = CI::parse("scheduler-start");
    check(b.command == "scheduler-start" && b.flag.empty() && b.argument.empty(),
          "'scheduler-start' is command only");

    auto c = CI::parse("");
    check(c.command.empty() && c.flag.empty() && c.argument.empty(),
          "blank line yields empty parse");

    auto d = CI::parse("   screen    -r    my proc  ");
    check(d.command == "screen" && d.flag == "-r" && d.argument == "my proc",
          "extra whitespace trimmed; multi-word argument preserved");

    auto e = CI::parse("process-smi");
    check(e.command == "process-smi" && e.flag.empty(),
          "'process-smi' recognized as a bare command");
}

// ---------------------------------------------------------------------------
static void test_gate_and_dispatch() {
    std::cout << "[gate + dispatch]\n";

    // ---- before initialize ----
    {
        std::ostringstream out;
        Config cfg;
        Console console(out);
        Scheduler sched(cfg, out);
        CommandInterpreter interp(cfg, console, sched, out);

        bool cont = interp.handle("screen -ls");
        check(cont, "non-exit command keeps the loop alive");
        check(contains(out.str(), "Please run 'initialize' first."),
              "commands blocked before initialize");
        check(!interp.isInitialized(), "not initialized yet");

        // exit works even before initialize
        check(interp.handle("exit") == false, "'exit' returns false before init");
    }

    // ---- unknown / errors after initialize ----
    {
        std::ostringstream out;
        Config cfg;
        Console console(out);
        Scheduler sched(cfg, out);
        CommandInterpreter interp(cfg, console, sched, out);

        interp.handle("initialize");
        check(interp.isInitialized(), "initialize sets the initialized flag");

        out.str("");
        interp.handle("foobar");
        check(contains(out.str(), "Unknown command: 'foobar'."),
              "unknown command reported");

        out.str("");
        interp.handle("screen -s");
        check(contains(out.str(), "requires a process name"),
              "missing argument for 'screen -s' reported");

        out.str("");
        interp.handle("screen -r ghost");
        check(contains(out.str(), "Process ghost not found."),
              "'screen -r' on missing process reports not found");

        // create then re-attach: no 'not found'
        out.str("");
        interp.handle("screen -s p01");
        interp.handle("screen -r p01");
        check(!contains(out.str(), "not found"),
              "'screen -r' finds a process created by 'screen -s'");
        check(sched.hasProcess("p01"), "scheduler registered the created process");

        // scheduler-start / stop dispatch
        out.str("");
        interp.handle("scheduler-start");
        check(sched.isRunning(), "scheduler-start starts the scheduler");
        interp.handle("scheduler-stop");
        check(!sched.isRunning(), "scheduler-stop stops the scheduler");

        // report-util dispatch
        out.str("");
        interp.handle("report-util");
        check(contains(out.str(), "csopesy-log.txt"), "report-util dispatched");
    }
}

// ---------------------------------------------------------------------------
static void test_config() {
    std::cout << "[config]\n";

    // valid config
    {
        std::ostringstream log;
        std::string path = writeTemp("test_cfg_valid.txt",
            "num-cpu 8\nscheduler \"fcfs\"\nquantum-cycles 10\n"
            "batch-process-freq 2\nmin-ins 500\nmax-ins 1500\ndelays-per-exec 3\n");
        Config c = Config::loadFromFile(path, log);
        check(c.numCpu == 8, "num-cpu read");
        check(c.scheduler == "fcfs", "scheduler read (quotes stripped)");
        check(c.quantumCycles == 10, "quantum-cycles read");
        check(c.batchProcessFreq == 2, "batch-process-freq read");
        check(c.minIns == 500 && c.maxIns == 1500, "min/max-ins read");
        check(c.delaysPerExec == 3, "delays-per-exec read");
        std::remove(path.c_str());
    }

    // out-of-range -> defaults
    {
        std::ostringstream log;
        std::string path = writeTemp("test_cfg_bad.txt",
            "num-cpu 999\nscheduler banana\nquantum-cycles 0\ndelays-per-exec abc\n");
        Config c = Config::loadFromFile(path, log);
        check(c.numCpu == 4, "num-cpu out of range falls back to default 4");
        check(c.scheduler == "rr", "invalid scheduler falls back to default rr");
        check(c.quantumCycles == 5, "quantum-cycles=0 (below range) falls back to 5");
        check(c.delaysPerExec == 0, "non-numeric delays-per-exec falls back to 0");
        check(contains(log.str(), "out of range") || contains(log.str(), "invalid"),
              "validation warnings logged");
        std::remove(path.c_str());
    }

    // missing file -> all defaults
    {
        std::ostringstream log;
        Config c = Config::loadFromFile("this_file_does_not_exist.txt", log);
        check(c.numCpu == 4 && c.scheduler == "rr" && c.minIns == 1000,
              "missing file uses all defaults");
        check(contains(log.str(), "could not open"), "missing file reported");
    }

    // accepts the sample spelling "delay-per-exec"
    {
        std::ostringstream log;
        std::string path = writeTemp("test_cfg_altkey.txt", "delay-per-exec 7\n");
        Config c = Config::loadFromFile(path, log);
        check(c.delaysPerExec == 7, "accepts 'delay-per-exec' spelling");
        std::remove(path.c_str());
    }
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "Running CSOPESY interpreter + config tests...\n";
    test_parse();
    test_gate_and_dispatch();
    test_config();

    std::cout << "----------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed.\n";
    if (g_failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    std::cout << g_failures << " CHECK(S) FAILED\n";
    return 1;
}
