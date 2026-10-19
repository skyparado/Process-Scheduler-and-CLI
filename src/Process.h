#pragma once
// ============================================================================
// Process  -  STUB
// Owner (to implement fully): the "Process + Instructions" teammate.
//
// Sky's modules only need a name/id/finished flag to dispatch commands. The
// instruction set (PRINT/DECLARE/ADD/SUBTRACT/SLEEP/FOR), variables, logs, and
// the uint16 clamp live here and are filled in by the Process teammate.
// ============================================================================
#include <string>
#include <cstdint>

struct Process {
    std::string name;
    int         pid      = 0;
    bool        finished = false;

    // TODO (Process teammate): instructions, currentLine, totalLines,
    // variables map, logs, etc.

    // uint16 variables are clamped between 0 and 65535 (see spec). Helper kept
    // here because clamping is the Process module's responsibility.
    static uint16_t clampU16(long long value) {
        if (value < 0) return 0;
        if (value > 65535) return 65535;
        return static_cast<uint16_t>(value);
    }
};
