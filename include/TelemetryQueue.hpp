#pragma once

#include <stdint.h>

struct TelemetryMessage
{
    float accelerationG;
    float forceN;
    float currentA;
    float temperatureC;

    uint8_t safeMode;
};
