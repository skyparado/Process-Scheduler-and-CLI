#pragma once
// ============================================================================
// Process  -  CSOPESY Emulator
// Owner: Theo  (Process + Instructions module)
//
// Stores a process's name, ID, instructions, current line, total lines,
// variables, and logs, and executes its instructions one line at a time:
// PRINT, DECLARE, ADD, SUBTRACT, SLEEP, and FOR (nestable up to 3 levels).
//
// The Scheduler decides when a process runs and on which core; this module only
// knows what running the next line does. It takes no locks: if the Scheduler
// runs on its own thread, it must guard these calls and the Console's reads.
// ============================================================================
#include <string>
#include <cstdint>
#include <cstddef>
#include <map>
#include <vector>

enum class InstructionType { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR };

// An instruction's source value: a variable name, or a literal if `var` is empty.
struct Operand {
    std::string var;
    long long   value = 0;

    Operand() = default;
    Operand(const char* name) : var(name) {}
    Operand(int literal) : value(literal) {}
};

struct Instruction {
    InstructionType type = InstructionType::PRINT;
    std::string var;                // DECLARE/ADD/SUBTRACT target; PRINT's variable
    std::string msg;                // PRINT text; empty means the default message
    Operand     a, b;               // sources (DECLARE only uses a)
    uint8_t     ticks = 0;          // SLEEP duration
    uint64_t    repeats = 0;        // FOR iterations
    std::vector<Instruction> body;  // FOR instructions

    static Instruction Print(const std::string& msg = "", const std::string& var = "");
    static Instruction Declare(const std::string& var, int value);
    static Instruction Add(const std::string& var, Operand a, Operand b);
    static Instruction Subtract(const std::string& var, Operand a, Operand b);
    static Instruction Sleep(uint8_t ticks);
    static Instruction For(std::vector<Instruction> body, uint64_t repeats);
};

struct Process {
    std::string name;
    int         pid      = 0;
    bool        finished = false;
    std::size_t currentLine = 0;    // lines executed so far
    std::size_t totalLines = 0;     // lines to execute; a FOR counts body * repeats
    std::vector<std::string> logs;  // formatted PRINT output
    std::vector<Instruction> instructions;
    std::map<std::string, uint16_t> variables;
    uint8_t     sleepTicks = 0;     // > 0: asleep, the Scheduler must free its core

    // Load a program. Returns false and changes nothing if FORs nest too deeply.
    bool setInstructions(std::vector<Instruction> program);

    // Load a random program of minIns..maxIns lines (in either order).
    void generateInstructions(uint64_t minIns, uint64_t maxIns);

    // Execute the next line on `core`: call once per CPU tick the process runs.
    // Does nothing while the process is asleep or finished.
    void executeNext(int core);

    // Count one CPU tick of sleep: call once per tick while sleepTicks > 0.
    void tickSleep();

    // uint16 variables are clamped between 0 and 65535 (see spec). Helper kept
    // here because clamping is the Process module's responsibility.
    static uint16_t clampU16(long long value) {
        if (value < 0) return 0;
        if (value > 65535) return 65535;
        return static_cast<uint16_t>(value);
    }

private:
    // One block being run: frames[0] is the program, each later frame a FOR body.
    struct Frame {
        std::size_t pc;             // next instruction in this block
        uint64_t    repeatsLeft;    // iterations left, counting the current one
    };
    std::vector<Frame> frames;

    const std::vector<Instruction>& currentBlock() const;
    long long read(const Operand& operand);
    void run(const Instruction& ins, int core);
    void finishIfDone();
};
