// ============================================================================
// Process.cpp  -  Owner: Theo
// ============================================================================
#include "Process.h"
#include "Console.h"

#include <algorithm>
#include <random>
#include <utility>

namespace {

const int MAX_FOR_DEPTH = 3;

// Generated SLEEPs stay short so processes still finish in a demo; hand-built
// programs can use the full uint8 range.
const uint64_t MAX_RANDOM_SLEEP = 10;

// Above this many lines, a program is stored as one big FOR. min-ins and max-ins
// go up to 2^32, which would not fit in memory as individual instructions.
const uint64_t MAX_STORED_LINES = 4096;

uint64_t pick(uint64_t lo, uint64_t hi) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    return std::uniform_int_distribution<uint64_t>(lo, hi)(rng);
}

const char* randomVar() {
    static const char* const names[] = {"x", "y", "z", "a", "b", "c"};
    return names[pick(0, 5)];
}

int randomValue() {
    return static_cast<int>(pick(0, 65535));
}

Operand randomOperand() {
    return pick(0, 1) ? Operand(randomVar()) : Operand(randomValue());
}

Instruction randomInstruction() {
    switch (pick(0, 4)) {
        case 0:  return Instruction::Declare(randomVar(), randomValue());
        case 1:  return Instruction::Add(randomVar(), randomOperand(), randomOperand());
        case 2:  return Instruction::Subtract(randomVar(), randomOperand(), randomOperand());
        case 3:  return Instruction::Sleep(static_cast<uint8_t>(pick(1, MAX_RANDOM_SLEEP)));
        default: return Instruction::Print();
    }
}

// Append instructions that execute exactly `lines` lines.
void generateBlock(std::vector<Instruction>& block, uint64_t lines, int depth) {
    while (lines > 0) {
        const bool huge = lines > MAX_STORED_LINES;
        if (depth < MAX_FOR_DEPTH && lines >= 2 && (huge || pick(0, 5) == 0)) {
            const uint64_t bodyLines = huge ? MAX_STORED_LINES
                                            : pick(1, std::min<uint64_t>(lines / 2, 10));
            const uint64_t repeats = huge ? lines / bodyLines
                                          : pick(2, std::min<uint64_t>(lines / bodyLines, 5));
            std::vector<Instruction> body;
            generateBlock(body, bodyLines, depth + 1);
            block.push_back(Instruction::For(std::move(body), repeats));
            lines -= bodyLines * repeats;
        } else {
            block.push_back(randomInstruction());
            --lines;
        }
    }
}

// `lines` lines of PRINT, ADD, PRINT, ..., always starting on a PRINT.
std::vector<Instruction> alternatingLines(uint64_t lines) {
    std::vector<Instruction> block;
    for (uint64_t i = 0; i < lines; ++i) {
        if (i % 2 == 0) block.push_back(Instruction::Print("Value from: ", "x"));
        else block.push_back(Instruction::Add("x", "x", static_cast<int>(pick(1, 10))));
    }
    return block;
}

int forDepth(const std::vector<Instruction>& block) {
    int deepest = 0;
    for (const Instruction& ins : block)
        if (ins.type == InstructionType::FOR)
            deepest = std::max(deepest, 1 + forDepth(ins.body));
    return deepest;
}

// A FOR takes no CPU tick itself; only the lines in its body do.
uint64_t countLines(const std::vector<Instruction>& block) {
    uint64_t lines = 0;
    for (const Instruction& ins : block)
        lines += ins.type == InstructionType::FOR ? ins.repeats * countLines(ins.body) : 1;
    return lines;
}

} // namespace

// ----------------------------------------------------------------------------
Instruction Instruction::Print(const std::string& msg, const std::string& var) {
    Instruction ins;
    ins.type = InstructionType::PRINT;
    ins.msg = msg;
    ins.var = var;
    return ins;
}

Instruction Instruction::Declare(const std::string& var, int value) {
    Instruction ins;
    ins.type = InstructionType::DECLARE;
    ins.var = var;
    ins.a = value;
    return ins;
}

Instruction Instruction::Add(const std::string& var, Operand a, Operand b) {
    Instruction ins;
    ins.type = InstructionType::ADD;
    ins.var = var;
    ins.a = std::move(a);
    ins.b = std::move(b);
    return ins;
}

Instruction Instruction::Subtract(const std::string& var, Operand a, Operand b) {
    Instruction ins = Add(var, std::move(a), std::move(b));
    ins.type = InstructionType::SUBTRACT;
    return ins;
}

Instruction Instruction::Sleep(uint8_t ticks) {
    Instruction ins;
    ins.type = InstructionType::SLEEP;
    ins.ticks = ticks;
    return ins;
}

Instruction Instruction::For(std::vector<Instruction> body, uint64_t repeats) {
    Instruction ins;
    ins.type = InstructionType::FOR;
    ins.body = std::move(body);
    ins.repeats = repeats;
    return ins;
}

// ----------------------------------------------------------------------------
bool Process::setInstructions(std::vector<Instruction> program) {
    if (forDepth(program) > MAX_FOR_DEPTH) return false;
    instructions = std::move(program);
    frames = {Frame{0, 1}};
    currentLine = 0;
    totalLines = static_cast<std::size_t>(countLines(instructions));
    finished = false;
    sleepTicks = 0;
    finishIfDone();
    return true;
}

void Process::generateInstructions(uint64_t minIns, uint64_t maxIns) {
    const auto range = std::minmax(minIns, maxIns);
    std::vector<Instruction> program;
    generateBlock(program, pick(range.first, range.second), 0);
    setInstructions(std::move(program));
}

void Process::generateAlternating(uint64_t minIns, uint64_t maxIns) {
    const auto range = std::minmax(minIns, maxIns);
    const uint64_t lines = pick(range.first, range.second);

    // Long programs repeat one stored block so they fit in memory, like generateBlock.
    // MAX_STORED_LINES is even, so the PRINT/ADD order holds across every repeat.
    std::vector<Instruction> program;
    if (lines >= MAX_STORED_LINES)
        program.push_back(Instruction::For(alternatingLines(MAX_STORED_LINES),
                                           lines / MAX_STORED_LINES));
    for (Instruction& ins : alternatingLines(lines % MAX_STORED_LINES))
        program.push_back(std::move(ins));

    variables["x"] = 0;
    setInstructions(std::move(program));
}

// ----------------------------------------------------------------------------
// Frames store indices rather than pointers so a Process stays valid when the
// Scheduler copies or moves it. frames[i + 1] runs the FOR at frames[i].pc - 1.
const std::vector<Instruction>& Process::currentBlock() const {
    const std::vector<Instruction>* block = &instructions;
    for (std::size_t i = 0; i + 1 < frames.size(); ++i)
        block = &(*block)[frames[i].pc - 1].body;
    return *block;
}

void Process::executeNext(int core) {
    if (finished || sleepTicks > 0) return;

    while (!frames.empty()) {
        Frame& top = frames.back();
        const std::vector<Instruction>& block = currentBlock();

        if (top.pc == block.size()) {           // end of block: go again or leave
            top.pc = 0;
            if (--top.repeatsLeft == 0) frames.pop_back();
            continue;
        }

        const Instruction& ins = block[top.pc++];
        if (ins.type == InstructionType::FOR) {
            if (ins.repeats > 0 && !ins.body.empty())
                frames.push_back(Frame{0, ins.repeats});
            continue;
        }

        run(ins, core);
        ++currentLine;
        finishIfDone();
        return;
    }
    finished = true;                            // no program was ever loaded
}

void Process::tickSleep() {
    if (sleepTicks > 0 && --sleepTicks == 0) finishIfDone();
}

// operator[] is what auto-declares an undeclared variable as 0.
long long Process::read(const Operand& operand) {
    return operand.var.empty() ? operand.value : variables[operand.var];
}

void Process::run(const Instruction& ins, int core) {
    switch (ins.type) {
        case InstructionType::PRINT: {
            std::string text = ins.msg.empty() ? "Hello world from " + name + "!" : ins.msg;
            if (!ins.var.empty()) text += std::to_string(variables[ins.var]);
            logs.push_back("(" + timestamp() + ") Core:" + std::to_string(core)
                           + " \"" + text + "\"");
            break;
        }
        case InstructionType::DECLARE:
            variables[ins.var] = clampU16(read(ins.a));
            break;
        case InstructionType::ADD:
            variables[ins.var] = clampU16(read(ins.a) + read(ins.b));
            break;
        case InstructionType::SUBTRACT:
            variables[ins.var] = clampU16(read(ins.a) - read(ins.b));
            break;
        case InstructionType::SLEEP:
            sleepTicks = ins.ticks;
            break;
        case InstructionType::FOR:              // handled by executeNext
            break;
    }
}

// Spec: variables stay in memory until the process finishes. The instructions go
// too, since a backlog of finished processes would otherwise keep them all. Logs
// and line counts stay because process-smi still shows them.
void Process::finishIfDone() {
    if (sleepTicks > 0 || currentLine < totalLines) return;
    finished = true;
    std::vector<Instruction>().swap(instructions);
    frames.clear();
    variables.clear();
}
