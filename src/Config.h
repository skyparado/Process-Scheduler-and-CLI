#pragma once
// ============================================================================
// Config  -  CSOPESY Emulator
// Owner: Sky  (Command Interpreter + Config module)
//
// Reads the space-separated "config.txt" for the CPU scheduler and process
// attributes, validates every value against its allowed range, and falls back
// to the spec default whenever a value is missing or invalid.
// ============================================================================
#include <string>
#include <cstdint>
#include <iosfwd>

struct Config {
    // --- Defaults (from the sample config.txt in the spec) ---
    int         numCpu           = 4;      // num-cpu            [1, 128]
    std::string scheduler        = "rr";   // scheduler          {fcfs, rr}
    uint64_t    quantumCycles    = 5;      // quantum-cycles     [1, 2^32]
    uint64_t    batchProcessFreq = 1;      // batch-process-freq [1, 2^32]
    uint64_t    minIns           = 1000;   // min-ins            [1, 2^32]
    uint64_t    maxIns           = 2000;   // max-ins            [1, 2^32]
    uint64_t    delaysPerExec    = 0;      // delays-per-exec    [0, 2^32]

    // Inclusive upper bound used by the ranges above (2^32).
    static constexpr uint64_t U32_CEIL = 4294967296ULL;

    // Load config from `path`. Any missing/invalid key keeps the default value
    // and a note is written to `log`. The returned Config always holds a valid,
    // usable set of parameters. If the file cannot be opened, all defaults are
    // used and that is reported on `log`.
    static Config loadFromFile(const std::string& path, std::ostream& log);
};
