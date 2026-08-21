#include "CoilDriver.hpp"


/* ============================================================
 * SET COIL CURRENT
 * ============================================================ */

CoilResult CoilDriver::setCurrent(
    float requestedCurrentAmps,
    float temperatureCelsius
) noexcept
{
    /* ========================================================
     * OVER TEMPERATURE
     * ======================================================== */

    if (temperatureCelsius >=
        MAX_SAFE_TEMP_CELSIUS)
    {
        currentAmps_ =
            0.0f;

        return CoilResult{
            CoilError::OverTemperature,
            0.0f
        };
    }


    /* ========================================================
     * NEGATIVE CURRENT
     *
     * One-directional actuator.
     *
     * Negative request is safely clamped to zero.
     * ======================================================== */

    if (requestedCurrentAmps <
        MIN_SAFE_CURRENT_AMPS)
    {
        currentAmps_ =
            MIN_SAFE_CURRENT_AMPS;

        return CoilResult{
            CoilError::None,
            currentAmps_
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
 * GENERIC COMMAND INTERFACE
 * ============================================================ */

ErrorCode CoilDriver::apply(
    const DampingCommand& command
)
{
    if (command.coilCurrentAmps <
        MIN_SAFE_CURRENT_AMPS)
    {
        currentAmps_ =
            MIN_SAFE_CURRENT_AMPS;

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
 * CURRENT
 * ============================================================ */

float CoilDriver::current() const noexcept
{
    return currentAmps_;
}


/* ============================================================
 * QEMU OVER-CURRENT TEST
 *
 * 6 A requested
 * 5 A maximum safe current
 *
 * Expected:
 *
 * requested = 6.0 A
 * actual    = 5.0 A
 * error     = OverCurrent
 * ============================================================ */

CoilResult CoilDriver::injectOverCurrentTest() noexcept
{
    constexpr float TEST_CURRENT_AMPS =
        6.0f;

    return setCurrent(
        TEST_CURRENT_AMPS,
        25.0f
    );
}