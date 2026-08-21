#ifndef TELEMETRY_DATA_HPP
#define TELEMETRY_DATA_HPP

#include <stdint.h>

struct TelemetryData
{
    float accelerationG;
    float forceN;
    float currentA;
    float temperatureC;

    bool safeMode;

    uint8_t sensorError;
    uint8_t coilError;

    uint32_t timestampMs;
};

#endif