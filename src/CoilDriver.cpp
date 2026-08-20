#include "CoilDriver.hpp"


/* ============================================================
 * Set coil current
 * ============================================================ */

CoilResult CoilDriver::setCurrent(
    float requestedCurrentAmps,
    float temperatureCelsius) noexcept
{
    /* ========================================================
     * OVER TEMPERATURE
     * ======================================================== */

    if (temperatureCelsius >=
        MAX_SAFE_TEMP_CELSIUS)
    {
        currentAmps_ = 0.0f;

        return CoilResult{
            CoilError::OverTemperature,
            0.0f
        };
    }


    /* ========================================================
     * NEGATIVE CURRENT
     *
     * This actuator is one-directional.
     *
     * Example:
     *
     *     -4 N force
     *          ↓
     *     requested current = -0.04 A
     *          ↓
     *     clamp to 0 A
     * ======================================================== */

    if (requestedCurrentAmps < 0.0f)
    {
        currentAmps_ = 0.0f;

        return CoilResult{
            CoilError::None,
            0.0f
        };
    }


    /* ========================================================
     * OVER CURRENT
     * ======================================================== */

    if (requestedCurrentAmps >
        MAX_SAFE_CURRENT_AMPS)
    {
        currentAmps_ =
            MAX_SAFE_CURRENT_AMPS;

        return CoilResult{
            CoilError::OverCurrent,
            currentAmps_
        };
    }


    /* ========================================================
     * NORMAL
     * ======================================================== */

    currentAmps_ =
        requestedCurrentAmps;

    return CoilResult{
        CoilError::None,
        currentAmps_
    };
}


/* ============================================================
 * Generic command interface
 * ============================================================ */

ErrorCode CoilDriver::apply(
    const DampingCommand& command)
{
    if (command.coilCurrentAmps < 0.0f)
    {
        currentAmps_ = 0.0f;

        return ErrorCode::COIL_INVALID_NEGATIVE_CURRENT;
    }


    if (command.coilCurrentAmps >
        MAX_SAFE_CURRENT_AMPS)
    {
        currentAmps_ =
            MAX_SAFE_CURRENT_AMPS;

        return ErrorCode::COIL_OVERCURRENT_CLAMPED;
    }


    currentAmps_ =
        command.coilCurrentAmps;

    return ErrorCode::SUCCESS;
}


/* ============================================================
 * Get actual current
 * ============================================================ */

float CoilDriver::current() const noexcept
{
    return currentAmps_;
}


/* ============================================================
 * QEMU OVER-CURRENT TEST
 * ============================================================ */

CoilResult CoilDriver::injectOverCurrentTest() noexcept
{
    constexpr float TEST_CURRENT_AMPS = 6.0f;

    return setCurrent(
        TEST_CURRENT_AMPS,
        25.0f
    );
}