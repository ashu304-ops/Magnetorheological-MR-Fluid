#include "TelemetryFormatter.hpp"

#include <cstdio>

std::string TelemetryFormatter::format(
    const TelemetryRecord& record)
{
    char buffer[128];

    std::snprintf(
        buffer,
        sizeof(buffer),
        "[TEL] acc=%.2fg force=%.2fN current=%.3fA\r\n",
        record.accelerationG,
        record.forceNewton,
        record.coilCurrentAmps
    );

    return std::string(buffer);
}