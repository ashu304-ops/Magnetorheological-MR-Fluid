#pragma once

#include "ITelemetryLogger.hpp"
#include "TelemetryRecord.hpp"

class TelemetryLogger : public ITelemetryLogger
{
public:

    void record(
        const SensorReadResult& sensor,
        const CoilResult& coil,
        float forceN
    ) noexcept override;

    void recordSensorError(
        SensorError error
    ) noexcept override;

    void recordCoilError(
        CoilError error
    ) noexcept override;

    [[nodiscard]]
    TelemetryRecord getLatest() const noexcept;

    [[nodiscard]]
    SensorError getLastSensorError() const noexcept;

    [[nodiscard]]
    CoilError getLastCoilError() const noexcept;

private:

    TelemetryRecord latest_{};

    SensorError lastSensorError_{
        SensorError::None
    };

    CoilError lastCoilError_{
        CoilError::None
    };
};