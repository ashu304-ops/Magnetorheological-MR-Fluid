#include "TelemetryLogger.hpp"

void TelemetryLogger::record(
    const SensorReadResult& sensor,
    const CoilResult& coil,
    float forceN) noexcept
{
    latest_.accelerationG = sensor.accelerationG;
    latest_.forceNewton = forceN;
    latest_.coilCurrentAmps = coil.actualCurrentAmps;

    lastSensorError_ = sensor.error;
    lastCoilError_ = coil.error;
}

void TelemetryLogger::recordSensorError(
    SensorError error) noexcept
{
    lastSensorError_ = error;
    latest_.coilCurrentAmps = 0.0f;
}

void TelemetryLogger::recordCoilError(
    CoilError error) noexcept
{
    lastCoilError_ = error;
    latest_.coilCurrentAmps = 0.0f;
}

TelemetryRecord TelemetryLogger::getLatest() const noexcept
{
    return latest_;
}

SensorError TelemetryLogger::getLastSensorError() const noexcept
{
    return lastSensorError_;
}

CoilError TelemetryLogger::getLastCoilError() const noexcept
{
    return lastCoilError_;
}