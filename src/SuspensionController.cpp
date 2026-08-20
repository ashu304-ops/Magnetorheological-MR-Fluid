
#include "SuspensionController.hpp"
#include "ComfortStrategy.hpp"

SuspensionController::SuspensionController(
    ISensorReader& sensor,
    ICoilDriver& coil,
    ITelemetryLogger& logger) noexcept
    : sensor_(sensor),
      coil_(coil),
      logger_(logger)
{
}

void SuspensionController::setStrategy(
    std::unique_ptr<IDampingStrategy> strategy) noexcept
{
    strategy_ = std::move(strategy);
}

void SuspensionController::runCycle(float ambientTempC) noexcept
{
    lastSensorError_ = SensorError::None;
    lastCoilError_ = CoilError::None;
    safeMode_ = false;

    // ------------------------------------------------------------
    // Simulated thermal model
    // ------------------------------------------------------------

    constexpr float dt = 0.01f;

    const float currentSquared =
        lastRequestedCurrentA_ * lastRequestedCurrentA_;

    const float heatingRate =
        currentSquared * 15.0f;

    const float coolingRate =
        (lastTemperatureC_ - ambientTempC) * 0.40f;

    lastTemperatureC_ +=
        (heatingRate - coolingRate) * dt;

    // Keep temperature above ambient.
    if (lastTemperatureC_ < ambientTempC)
    {
        lastTemperatureC_ = ambientTempC;
    }

    // ------------------------------------------------------------
    // Thermal safety limit
    //
    // Use the same 85 C limit defined in ErrorTypes.hpp.
    // ------------------------------------------------------------

    if (lastTemperatureC_ >= MAX_SAFE_TEMP_CELSIUS)
    {
        enterSafeMode(SensorError::HardwareFault);
        return;
    }

    // ------------------------------------------------------------
    // Make sure a strategy exists
    // ------------------------------------------------------------

    if (!strategy_)
    {
        strategy_ =
            std::make_unique<ComfortStrategy>();
    }

    // ------------------------------------------------------------
    // Read acceleration sensor
    // ------------------------------------------------------------

    const SensorReadResult sensorResult =
        sensor_.read();

    lastAccelerationG_ =
        sensorResult.accelerationG;

    lastSensorError_ =
        sensorResult.error;

    if (!sensorResult.success()) {
    filter_.reset();

    enterSafeMode(sensorResult.error);
    return;
}

    // ------------------------------------------------------------
    // Filter sensor value
    // ------------------------------------------------------------

    const float filteredG =
        filter_.filter(
            sensorResult.accelerationG);

    // ------------------------------------------------------------
    // Calculate damping force
    // ------------------------------------------------------------

    const float forceN =
        strategy_->calculateForceN(
            filteredG);

    lastForceN_ =
        forceN;

    // Store force history.
    forceHistory_.push(forceN);

    // ------------------------------------------------------------
    // Convert force to requested coil current
    // ------------------------------------------------------------

    const float requestedCurrentA =
        forceN * 0.01f;

    // ------------------------------------------------------------
    // Apply current through CoilDriver
    //
    // CoilDriver is responsible for:
    //
    //   Negative current  -> 0 A
    //   Over-current      -> 5 A maximum
    //   Over-temperature  -> 0 A
    //
    // ------------------------------------------------------------

    const CoilResult coilResult =
        coil_.setCurrent(
            requestedCurrentA,
            lastTemperatureC_);

    // ------------------------------------------------------------
    // IMPORTANT:
    //
    // Store ACTUAL current returned by CoilDriver,
    // not the requested current.
    // ------------------------------------------------------------

    lastRequestedCurrentA_ =
        coilResult.actualCurrentAmps;

    lastCoilError_ =
        coilResult.error;

    // ------------------------------------------------------------
    // Coil fault
    // ------------------------------------------------------------

    if (!coilResult.success())
    {
        enterSafeMode(
            SensorError::HardwareFault);

        return;
    }

    // ------------------------------------------------------------
    // Normal telemetry
    // ------------------------------------------------------------

    logger_.record(
        sensorResult,
        coilResult,
        forceN);
}

void SuspensionController::enterSafeMode(
    SensorError error) noexcept
{
    safeMode_ = true;

    lastSensorError_ =
        error;

    // Force coil current to zero.
    const CoilResult result =
        coil_.setCurrent(
            0.0f,
            lastTemperatureC_);

    lastRequestedCurrentA_ =
        result.actualCurrentAmps;

    lastCoilError_ =
        result.error;

    logger_.recordSensorError(
        error);
}

