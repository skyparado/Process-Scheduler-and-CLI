// ============================================================================
// Config.cpp  -  Owner: Sky
// ============================================================================
#include "Config.h"

#include <fstream>
#include <sstream>
#include <ostream>
#include <map>
#include <string>
#include <cctype>

namespace {

// Strip surrounding double quotes and whitespace from a token: "rr" -> rr
std::string unquote(std::string s) {
    // trim whitespace
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    s = s.substr(a, b - a + 1);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);
    return s;
}

// Parse an unsigned 64-bit integer. Returns false on any non-numeric input.
bool parseU64(const std::string& tok, uint64_t& out) {
    if (tok.empty()) return false;
    for (char c : tok)
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    try {
        out = std::stoull(tok);
    } catch (...) {
        return false;                 // overflow or bad parse
    }
    return true;
}

// Validate `tok` as a uint in [lo, hi]; on success assign to `field`, otherwise
// leave `field` at its default and log why.
void takeU64(const std::string& key, const std::string& tok,
             uint64_t lo, uint64_t hi, uint64_t& field, std::ostream& log) {
    uint64_t v = 0;
    if (!parseU64(tok, v)) {
        log << "[config] '" << key << "' value '" << tok
            << "' is not a valid number; using default " << field << ".\n";
        return;
    }
    if (v < lo || v > hi) {
        log << "[config] '" << key << "' value " << v << " is out of range ["
            << lo << ", " << hi << "]; using default " << field << ".\n";
        return;
    }
    field = v;
}

} // namespace

Config Config::loadFromFile(const std::string& path, std::ostream& log) {
    Config cfg;  // starts at defaults

    std::ifstream in(path);
    if (!in) {
        log << "[config] could not open '" << path
            << "'; using all default parameters.\n";
        return cfg;
    }

    // Collect key -> value (value = everything after the first token on the line).
    std::map<std::string, std::string> kv;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;             // blank line
        if (key.empty() || key[0] == '#') continue;  // comment
        std::string rest;
        std::getline(ls, rest);
        kv[key] = unquote(rest);
    }

    auto has = [&](const std::string& k) { return kv.find(k) != kv.end(); };

    // --- num-cpu  [1, 128] ---
    if (has("num-cpu")) {
        uint64_t v = 0;
        if (parseU64(kv["num-cpu"], v) && v >= 1 && v <= 128) {
            cfg.numCpu = static_cast<int>(v);
        } else {
            log << "[config] 'num-cpu' value '" << kv["num-cpu"]
                << "' invalid (expected 1..128); using default " << cfg.numCpu << ".\n";
        }
    } else {
        log << "[config] 'num-cpu' missing; using default " << cfg.numCpu << ".\n";
    }

    // --- scheduler  {fcfs, rr} ---
    if (has("scheduler")) {
        std::string s = kv["scheduler"];
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (s == "fcfs" || s == "rr") {
            cfg.scheduler = s;
        } else {
            log << "[config] 'scheduler' value '" << kv["scheduler"]
                << "' invalid (expected fcfs or rr); using default " << cfg.scheduler << ".\n";
        }
    } else {
        log << "[config] 'scheduler' missing; using default " << cfg.scheduler << ".\n";
    }

    // --- cycle / instruction counts ---
    if (has("quantum-cycles"))
        takeU64("quantum-cycles", kv["quantum-cycles"], 1, U32_CEIL, cfg.quantumCycles, log);
    else
        log << "[config] 'quantum-cycles' missing; using default " << cfg.quantumCycles << ".\n";

    if (has("batch-process-freq"))
        takeU64("batch-process-freq", kv["batch-process-freq"], 1, U32_CEIL, cfg.batchProcessFreq, log);
    else
        log << "[config] 'batch-process-freq' missing; using default " << cfg.batchProcessFreq << ".\n";

    if (has("min-ins"))
        takeU64("min-ins", kv["min-ins"], 1, U32_CEIL, cfg.minIns, log);
    else
        log << "[config] 'min-ins' missing; using default " << cfg.minIns << ".\n";

    if (has("max-ins"))
        takeU64("max-ins", kv["max-ins"], 1, U32_CEIL, cfg.maxIns, log);
    else
        log << "[config] 'max-ins' missing; using default " << cfg.maxIns << ".\n";

    // delays-per-exec  [0, 2^32]  (accept the sample's "delay-per-exec" spelling too)
    std::string delayKey = has("delays-per-exec") ? "delays-per-exec"
                         : (has("delay-per-exec") ? "delay-per-exec" : "");
    if (!delayKey.empty())
        takeU64(delayKey, kv[delayKey], 0, U32_CEIL, cfg.delaysPerExec, log);
    else
        log << "[config] 'delays-per-exec' missing; using default " << cfg.delaysPerExec << ".\n";

    return cfg;
}
