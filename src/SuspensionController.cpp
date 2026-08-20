#include "SuspensionController.hpp"

#include "ComfortStrategy.hpp"
#include "Hardware.hpp"


extern "C" void hw_uart_puts(const char* str);


/* ============================================================
 * CONSTRUCTOR
 * ============================================================ */

SuspensionController::SuspensionController(
    ISensorReader& sensor,
    ICoilDriver& coil,
    ITelemetryLogger& logger
) noexcept
    : sensor_(sensor),
      coil_(coil),
      logger_(logger)
{
}


/* ============================================================
 * SET STRATEGY
 * ============================================================ */

void SuspensionController::setStrategy(
    std::unique_ptr<IDampingStrategy> strategy
) noexcept
{
    strategy_ =
        std::move(strategy);
}


/* ============================================================
 * CONTROL CYCLE
 *
 * 100 Hz
 *
 *                    ┌──────────────┐
 *                    │    NORMAL    │
 *                    └──────┬───────┘
 *                           │
 *                sensor fault / coil fault
 *                    ┌──────┴──────┐
 *                    ▼             ▼
 *             SENSOR SAFE      COIL SAFE
 *                    │             │
 *             sensor recovered    │
 *                    │       explicit clear
 *                    ▼             │
 *                  NORMAL <────────┘
 *
 * SENSOR SAFE:
 *      Automatically recovers when sensor becomes healthy.
 *
 * COIL SAFE:
 *      Remains latched until clearCoilFault() is called.
 * ============================================================ */

void SuspensionController::runCycle(
    float ambientTempC
) noexcept
{
    /* ========================================================
     * THERMAL MODEL
     * ======================================================== */

    constexpr float dt = 0.01f;


    const float currentSquared =
        lastRequestedCurrentA_ *
        lastRequestedCurrentA_;


    const float heatingRate =
        currentSquared * 15.0f;


    const float coolingRate =
        (lastTemperatureC_ - ambientTempC) *
        0.40f;


    lastTemperatureC_ +=
        (heatingRate - coolingRate) * dt;


    if (lastTemperatureC_ < ambientTempC)
    {
        lastTemperatureC_ =
            ambientTempC;
    }


    /* ========================================================
     * THERMAL SAFETY
     * ======================================================== */

    if (lastTemperatureC_ >=
        MAX_SAFE_TEMP_CELSIUS)
    {
        enterSafeMode(
            CoilError::OverTemperature
        );

        return;
    }


    /* ========================================================
     * COIL SAFE MODE
     *
     * A coil fault is latched.
     *
     * Do not execute normal control until
     * clearCoilFault() is explicitly called.
     * ======================================================== */

    if (state_ ==
        ControllerState::CoilSafeMode)
    {
        return;
    }


    /* ========================================================
     * ENSURE STRATEGY EXISTS
     * ======================================================== */

    if (!strategy_)
    {
        strategy_ =
            std::make_unique<ComfortStrategy>();
    }


    /* ========================================================
     * READ SENSOR
     * ======================================================== */

    const SensorReadResult sensorResult =
        sensor_.read();


    lastAccelerationG_ =
        sensorResult.accelerationG;


    lastSensorError_ =
        sensorResult.error;


    /* ========================================================
     * SENSOR FAULT
     * ======================================================== */

    if (!sensorResult.success())
    {
        enterSafeMode(
            sensorResult.error
        );

        return;
    }


    /* ========================================================
     * SENSOR SAFE MODE RECOVERY
     * ======================================================== */

    if (state_ ==
        ControllerState::SensorSafeMode)
    {
        /*
         * Sensor is healthy again.
         *
         * Discard old filter history because it
         * may contain stale values from the fault.
         */

        filter_.reset();


        state_ =
            ControllerState::Normal;


        lastSensorError_ =
            SensorError::None;


        lastCoilError_ =
            CoilError::None;


        lastForceN_ =
            0.0f;


        lastRequestedCurrentA_ =
            0.0f;


        hw_uart_puts(
            "[CTRL] SENSOR RECOVERED - EXITING SAFE MODE\r\n"
        );
    }


    /* ========================================================
     * FILTER
     * ======================================================== */

    const float filteredG =
        filter_.filter(
            sensorResult.accelerationG
        );


    lastAccelerationG_ =
        filteredG;


    /* ========================================================
     * CALCULATE FORCE
     * ======================================================== */

    const float forceN =
        strategy_->calculateForceN(
            filteredG
        );


    lastForceN_ =
        forceN;


    /* ========================================================
     * FORCE HISTORY
     * ======================================================== */

    forceHistory_.push(
        forceN
    );


    /* ========================================================
     * FORCE -> CURRENT
     *
     * Example:
     *
     *   4 N  ->  0.04 A
     *  -4 N  -> -0.04 A
     * ======================================================== */

    const float requestedCurrentA =
        forceN * 0.01f;


    /* ========================================================
     * APPLY COIL CURRENT
     * ======================================================== */

    const CoilResult coilResult =
        coil_.setCurrent(
            requestedCurrentA,
            lastTemperatureC_
        );


    /* ========================================================
     * STORE ACTUAL CURRENT
     * ======================================================== */

    lastRequestedCurrentA_ =
        coilResult.actualCurrentAmps;


    lastCoilError_ =
        coilResult.error;


    /* ========================================================
     * COIL FAULT
     * ======================================================== */

    if (!coilResult.success())
    {
        enterSafeMode(
            coilResult.error
        );

        return;
    }


    /* ========================================================
     * NORMAL TELEMETRY
     * ======================================================== */

    logger_.record(
        sensorResult,
        coilResult,
        forceN
    );
}


/* ============================================================
 * ENTER SAFE MODE - SENSOR FAULT
 * ============================================================ */

void SuspensionController::enterSafeMode(
    SensorError error
) noexcept
{
    state_ =
        ControllerState::SensorSafeMode;


    lastSensorError_ =
        error;


    lastForceN_ =
        0.0f;


    lastRequestedCurrentA_ =
        0.0f;


    filter_.reset();


    /* --------------------------------------------------------
     * FORCE COIL OFF
     * -------------------------------------------------------- */

    const CoilResult result =
        coil_.setCurrent(
            0.0f,
            lastTemperatureC_
        );


    /*
     * The coil may successfully accept the
     * safe 0 A command.
     *
     * Therefore do not treat result.error as
     * the original sensor fault.
     */

    lastRequestedCurrentA_ =
        result.actualCurrentAmps;


    lastCoilError_ =
        result.error;


    logger_.recordSensorError(
        error
    );
}


/* ============================================================
 * ENTER SAFE MODE - COIL FAULT
 * ============================================================ */

void SuspensionController::enterSafeMode(
    CoilError error
) noexcept
{
    state_ =
        ControllerState::CoilSafeMode;


    /*
     * IMPORTANT:
     *
     * Preserve the original coil fault.
     *
     * This fault remains active until
     * clearCoilFault() is explicitly called.
     */

    lastCoilError_ =
        error;


    lastForceN_ =
        0.0f;


    lastRequestedCurrentA_ =
        0.0f;


    filter_.reset();


    /* --------------------------------------------------------
     * FORCE COIL OFF
     * -------------------------------------------------------- */

    const CoilResult result =
        coil_.setCurrent(
            0.0f,
            lastTemperatureC_
        );


    /*
     * Do NOT overwrite lastCoilError_ here.
     *
     * result.error belongs to the safe 0 A command.
     *
     * We need to preserve the original fault:
     *
     *     OVER_CURRENT
     *
     *     OVER_TEMPERATURE
     * -------------------------------------------------------- */

    lastRequestedCurrentA_ =
        result.actualCurrentAmps;


    logger_.recordCoilError(
        error
    );
}


/* ============================================================
 * CLEAR COIL FAULT
 *
 * Explicit recovery mechanism.
 *
 * COIL SAFE
 *     |
 *     | clearCoilFault()
 *     v
 * NORMAL
 * ============================================================ */

void SuspensionController::clearCoilFault() noexcept
{
    /*
     * Nothing to clear if we are not in
     * coil safe mode.
     */

    if (state_ !=
        ControllerState::CoilSafeMode)
    {
        return;
    }


    /* --------------------------------------------------------
     * Clear controller state.
     * -------------------------------------------------------- */

    state_ =
        ControllerState::Normal;


    lastCoilError_ =
        CoilError::None;


    lastSensorError_ =
        SensorError::None;


    lastForceN_ =
        0.0f;


    lastRequestedCurrentA_ =
        0.0f;


    filter_.reset();


    /* --------------------------------------------------------
     * Keep actuator OFF during the transition.
     *
     * Normal current will only be calculated
     * during the next control cycle.
     * -------------------------------------------------------- */

    const CoilResult result =
        coil_.setCurrent(
            0.0f,
            lastTemperatureC_
        );


    lastRequestedCurrentA_ =
        result.actualCurrentAmps;


    hw_uart_puts(
        "[CTRL] COIL FAULT CLEARED\r\n"
    );
}