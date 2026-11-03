// ============================================================================
// test_main.cpp  -  unit tests for Sky's Command Interpreter + Config module
//                   and Theo's Process + Instructions module
//
// Tiny assertion harness (no external framework). Builds as its own executable;
// does not need the teammates' real Console/Scheduler/Process code -- it uses
// the stubs, driving everything through injected string streams.
// ============================================================================
#include "../src/Config.h"
#include "../src/Console.h"
#include "../src/Scheduler.h"
#include "../src/CommandInterpreter.h"
#include "../src/Process.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdio>
#include <regex>
#include <set>
#include <vector>

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
// Run a process the way the Scheduler will: one line or one sleep tick at a time.
static void runToEnd(Process& p) {
    for (int guard = 0; guard < 1000000 && !p.finished; ++guard) {
        if (p.sleepTicks > 0) p.tickSleep();
        else p.executeNext(0);
    }
}

// Record the instruction types, deepest FOR nesting, and stored size of a block.
static void scan(const std::vector<Instruction>& block, int depth,
                 std::set<InstructionType>& seen, int& deepest, std::size_t& stored) {
    for (const Instruction& ins : block) {
        seen.insert(ins.type);
        ++stored;
        if (ins.type == InstructionType::FOR) {
            if (depth + 1 > deepest) deepest = depth + 1;
            scan(ins.body, depth + 1, seen, deepest, stored);
        }
    }
}

static void test_process() {
    std::cout << "[process]\n";
    using I = Instruction;

    // PRINT: default message, spec log format, custom message with one variable
    {
        Process p;
        p.name = "p01";
        p.setInstructions({I::Print(), I::Declare("x", 7), I::Print("Value from: ", "x")});
        p.executeNext(2);
        const std::regex format(
            R"(^\(\d{2}/\d{2}/\d{4} \d{2}:\d{2}:\d{2}[AP]M\) Core:2 "Hello world from p01!"$)");
        check(p.logs.size() == 1 && std::regex_match(p.logs[0], format),
              "PRINT logs the default message in the spec's format");
        runToEnd(p);
        check(p.logs.size() == 2 && contains(p.logs[1], "\"Value from: 7\""),
              "PRINT appends one variable to a custom message");
    }

    // DECLARE / ADD / SUBTRACT, auto-declare, clamp. The trailing PRINT keeps the
    // process unfinished so its variables can still be read.
    {
        Process p;
        p.setInstructions({
            I::Declare("a", 10),
            I::Add("b", "a", 5),
            I::Subtract("c", "b", "a"),
            I::Add("d", "ghost", 1),
            I::Add("high", 65535, 1),
            I::Subtract("low", 3, 5),
            I::Declare("big", 70000),
            I::Add("a", "a", "a"),
            I::Print()});
        for (int i = 0; i < 8; ++i) p.executeNext(0);
        check(p.variables["a"] == 20, "ADD can overwrite its own source");
        check(p.variables["b"] == 15, "ADD adds a variable and a literal");
        check(p.variables["c"] == 5, "SUBTRACT subtracts two variables");
        check(p.variables["d"] == 1 && p.variables.count("ghost") == 1
                  && p.variables["ghost"] == 0, "undeclared variable auto-declared as 0");
        check(p.variables["high"] == 65535, "ADD clamps at 65535");
        check(p.variables["low"] == 0, "SUBTRACT clamps at 0");
        check(p.variables["big"] == 65535, "DECLARE clamps at 65535");
    }

    // SLEEP: no execution until woken; a process cannot finish while asleep
    {
        Process p;
        p.setInstructions({I::Sleep(2), I::Print()});
        p.executeNext(0);
        check(p.sleepTicks == 2 && p.currentLine == 1, "SLEEP puts the process to sleep");
        p.executeNext(0);
        check(p.currentLine == 1 && p.logs.empty(), "a sleeping process executes nothing");
        p.tickSleep();
        p.tickSleep();
        check(p.sleepTicks == 0, "sleep ends after X ticks");
        p.executeNext(0);
        check(p.finished && p.logs.size() == 1, "process resumes after waking");

        Process q;
        q.setInstructions({I::Sleep(1)});
        q.executeNext(0);
        check(!q.finished, "process ending in SLEEP is not finished while asleep");
        q.tickSleep();
        check(q.finished, "process ending in SLEEP finishes when it wakes");
    }

    // FOR: 3 nested levels run and count correctly; a 4th level is refused
    {
        Process p;
        p.setInstructions({
            I::For({I::For({I::For({I::Add("n", "n", 1)}, 2)}, 3)}, 2),
            I::Print("n=", "n")});
        check(p.totalLines == 13, "total lines count FOR body * repeats");
        runToEnd(p);
        check(p.finished && p.currentLine == 13, "current line ends at total lines");
        check(p.logs.size() == 1 && contains(p.logs[0], "\"n=12\""),
              "3-level nested FOR runs its body 2*3*2 times");

        Process skip;
        skip.setInstructions({I::For({I::Print()}, 0), I::For({}, 5), I::Print()});
        runToEnd(skip);
        check(skip.finished && skip.logs.size() == 1, "empty and 0-repeat FORs are skipped");

        Process deep;
        const auto four = I::For({I::For({I::For({I::For({I::Print()}, 1)}, 1)}, 1)}, 1);
        check(!deep.setInstructions({four}) && deep.totalLines == 0,
              "FOR nested 4 levels is refused");
    }

    // A copy made in the middle of a FOR must not depend on the original
    {
        Process original;
        original.setInstructions({I::For({I::Print(), I::Print()}, 3)});
        for (int i = 0; i < 3; ++i) original.executeNext(0);
        Process copy = original;
        original = Process();
        runToEnd(copy);
        check(copy.finished && copy.logs.size() == 6, "copied process continues correctly");
    }

    // Finishing frees instructions and variables but keeps what process-smi shows
    {
        Process p;
        p.setInstructions({I::Declare("x", 1), I::Print()});
        runToEnd(p);
        check(p.instructions.empty() && p.variables.empty(),
              "instructions and variables released on finish");
        check(p.logs.size() == 1 && p.totalLines == 2, "logs and line counts kept on finish");
    }

    // Random programs: right length, valid nesting, all types, run to completion
    {
        bool inRange = true, completes = true;
        std::set<InstructionType> seen;
        int deepest = 0;
        for (int i = 0; i < 30; ++i) {
            Process p;
            p.name = "gen";
            p.generateInstructions(1000, 2000);
            inRange = inRange && p.totalLines >= 1000 && p.totalLines <= 2000;
            std::size_t stored = 0;
            scan(p.instructions, 0, seen, deepest, stored);
            runToEnd(p);
            completes = completes && p.finished && p.currentLine == p.totalLines;
        }
        check(inRange, "generated length is between min-ins and max-ins");
        check(completes, "generated programs run to completion");
        check(seen.size() == 6, "generated programs use all 6 instruction types");
        check(deepest == 3, "generated FORs nest up to 3 levels");

        Process one;
        one.generateInstructions(1, 1);
        check(one.totalLines == 1, "min-ins = max-ins = 1 gives exactly one line");

        Process swapped;
        swapped.generateInstructions(200, 100);
        check(swapped.totalLines >= 100 && swapped.totalLines <= 200,
              "min-ins above max-ins is still handled");

        Process huge;
        huge.generateInstructions(Config::U32_CEIL, Config::U32_CEIL);
        std::size_t stored = 0;
        scan(huge.instructions, 0, seen, deepest, stored);
        check(huge.totalLines == Config::U32_CEIL && stored < 10000,
              "2^32 lines fit in a small stored program");
    }

    // The Scheduler gives each process a real program sized from config
    {
        std::ostringstream out;
        Config cfg;
        cfg.minIns = 30;
        cfg.maxIns = 40;
        Scheduler sched(cfg, out);
        sched.createProcess("p01");
        const Process* p = sched.getProcess("p01");
        check(p != nullptr && p->totalLines >= 30 && p->totalLines <= 40 && p->logs.empty(),
              "createProcess generates instructions from min-ins and max-ins");
    }
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "Running CSOPESY interpreter + config + process tests...\n";
    test_parse();
    test_gate_and_dispatch();
    test_config();
    test_process();

    std::cout << "----------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed.\n";
    if (g_failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    std::cout << g_failures << " CHECK(S) FAILED\n";
    return 1;
}
