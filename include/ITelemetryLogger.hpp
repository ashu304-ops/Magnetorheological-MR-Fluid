#pragma once

#include "ErrorTypes.hpp"


class ITelemetryLogger
{
public:

    virtual ~ITelemetryLogger() = default;


    virtual void record(
        const SensorReadResult& sensor,
        const CoilResult& coil,
        float forceN
    ) noexcept = 0;


    virtual void recordSensorError(
        SensorError error
    ) noexcept = 0;


    virtual void recordCoilError(
        CoilError error
    ) noexcept = 0;
};