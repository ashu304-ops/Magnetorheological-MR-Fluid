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
 * RUN CONTROL CYCLE
 *
 * 100 Hz
 *
 * NORMAL
 *    |
 *    +---- Sensor fault --------> SENSOR SAFE MODE
 *    |                              |
 *    |                              +-- sensor recovery
 *    |                                      |
 *    |                                      v
 *    |                                    NORMAL
 *    |
 *    +---- Coil fault ----------> COIL SAFE MODE
 *                                   |
 *                                   +-- explicit clear
 *                                          |
 *                                          v
 *                                        NORMAL
 *
 * Thermal protection is treated as a coil fault.
 * ============================================================ */

void SuspensionController::runCycle(
    float ambientTempC
) noexcept
{
    /* ========================================================
     * CURRENT CYCLE ERROR RESET
     *
     * Do not erase a latched coil fault.
     * ======================================================== */

    if (!coilFaultLatched_)
    {
        lastCoilError_ =
            CoilError::None;
    }


    /* ========================================================
     * THERMAL MODEL
     * ======================================================== */

    constexpr float dt = 0.01f;


    const float currentSquared =
        lastRequestedCurrentA_ *
        lastRequestedCurrentA_;


    /*
     * Simple simulated I² heating model.
     */
    const float heatingRate =
        currentSquared * 15.0f;


    /*
     * Simple cooling model.
     */
    const float coolingRate =
        (lastTemperatureC_ - ambientTempC) *
        0.40f;


    lastTemperatureC_ +=
        (heatingRate - coolingRate) * dt;


    /*
     * Temperature must not fall below ambient.
     */
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
     * EXISTING COIL FAULT LATCH
     *
     * If a coil fault already occurred, do not continue
     * normal control operation.
     * ======================================================== */

    if (coilFaultLatched_)
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
        filter_.reset();

        enterSafeMode(
            sensorResult.error
        );

        return;
    }


    /* ========================================================
     * SENSOR SAFE MODE RECOVERY
     * ======================================================== */

    if (safeMode_)
    {
        /*
         * At this point the only remaining safe-mode source
         * should be a sensor fault.
         *
         * Coil faults are handled above by coilFaultLatched_.
         */

        filter_.reset();


        safeMode_ =
            false;


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
 * SENSOR SAFE MODE
 * ============================================================ */

void SuspensionController::enterSafeMode(
    SensorError error
) noexcept
{
    safeMode_ =
        true;


    /*
     * Sensor faults do not create a persistent coil latch.
     */
    coilFaultLatched_ =
        false;


    lastSensorError_ =
        error;


    lastForceN_ =
        0.0f;


    lastRequestedCurrentA_ =
        0.0f;


    filter_.reset();


    /* --------------------------------------------------------
     * Force coil OFF.
     * -------------------------------------------------------- */

    const CoilResult result =
        coil_.setCurrent(
            0.0f,
            lastTemperatureC_
        );


    lastRequestedCurrentA_ =
        result.actualCurrentAmps;


    /*
     * Preserve sensor error as the primary fault.
     *
     * Only expose a coil error if the safe 0A command itself
     * fails.
     */

    if (!result.success())
    {
        lastCoilError_ =
            result.error;
    }
    else
    {
        lastCoilError_ =
            CoilError::None;
    }


    logger_.recordSensorError(
        error
    );
}


/* ============================================================
 * COIL SAFE MODE
 * ============================================================ */

void SuspensionController::enterSafeMode(
    CoilError error
) noexcept
{
    safeMode_ =
        true;


    /*
     * ALL coil faults are latched.
     *
     * This includes:
     *
     * - OverCurrent
     * - OverTemperature
     * - HardwareFault
     */
    coilFaultLatched_ =
        true;


    lastCoilError_ =
        error;


    lastForceN_ =
        0.0f;


    lastRequestedCurrentA_ =
        0.0f;


    filter_.reset();


    /* --------------------------------------------------------
     * Force coil OFF.
     * -------------------------------------------------------- */

    const CoilResult result =
        coil_.setCurrent(
            0.0f,
            lastTemperatureC_
        );


    lastRequestedCurrentA_ =
        result.actualCurrentAmps;


    /*
     * IMPORTANT:
     *
     * Keep the ORIGINAL fault.
     *
     * Do not replace OverTemperature or OverCurrent with
     * the result of the safe 0A command.
     */

    logger_.recordCoilError(
        error
    );
}


/* ============================================================
 * CLEAR COIL FAULT
 *
 * Explicit recovery.
 * ============================================================ */

void SuspensionController::clearCoilFault() noexcept
{
    /* ========================================================
     * Nothing to clear
     * ======================================================== */

    if (!coilFaultLatched_)
    {
        return;
    }


    /* ========================================================
     * TEMPERATURE CHECK
     *
     * Never clear an over-temperature fault while the system
     * is still above the safe operating temperature.
     * ======================================================== */

    if (lastTemperatureC_ >=
        MAX_SAFE_TEMP_CELSIUS)
    {
        lastCoilError_ =
            CoilError::OverTemperature;


        safeMode_ =
            true;


        coilFaultLatched_ =
            true;


        lastForceN_ =
            0.0f;


        lastRequestedCurrentA_ =
            0.0f;


        hw_uart_puts(
            "[CTRL] COIL FAULT CLEAR BLOCKED - TEMPERATURE TOO HIGH\r\n"
        );

        return;
    }


    /* ========================================================
     * FORCE COIL OFF FIRST
     * ======================================================== */

    const CoilResult result =
        coil_.setCurrent(
            0.0f,
            lastTemperatureC_
        );


    /* ========================================================
     * SAFE OUTPUT FAILED
     * ======================================================== */

    if (!result.success())
    {
        lastCoilError_ =
            result.error;


        safeMode_ =
            true;


        coilFaultLatched_ =
            true;


        lastForceN_ =
            0.0f;


        lastRequestedCurrentA_ =
            result.actualCurrentAmps;


        hw_uart_puts(
            "[CTRL] COIL FAULT CLEAR FAILED\r\n"
        );

        return;
    }


    /* ========================================================
     * CLEAR FAULT
     * ======================================================== */

    coilFaultLatched_ =
        false;


    safeMode_ =
        false;


    lastCoilError_ =
        CoilError::None;


    lastSensorError_ =
        SensorError::None;


    lastForceN_ =
        0.0f;


    lastRequestedCurrentA_ =
        0.0f;


    filter_.reset();


    forceHistory_ =
        RingBuffer<float, 32>{};


    hw_uart_puts(
        "[CTRL] COIL FAULT CLEARED - EXITING SAFE MODE\r\n"
    );
}


/* ============================================================
 * QEMU OVER-TEMPERATURE TEST
 *
 * TEST ONLY.
 *
 * Force temperature to the safety threshold so the next
 * control cycle exercises the actual thermal safety logic.
 * ============================================================ */


