
#include "CoilDriver.hpp"


CoilResult CoilDriver::setCurrent(
    float requestedCurrentAmps,
    float temperatureCelsius) noexcept
{
    // Thermal shutdown guardrail
    if (temperatureCelsius >= MAX_SAFE_TEMP_CELSIUS)
    {
        currentAmps_ = 0.0f;

        return CoilResult{
            CoilError::OverTemperature,
            0.0f
        };
    }

    // Over-current guardrail
    if (requestedCurrentAmps > MAX_SAFE_CURRENT_AMPS)
    {
        currentAmps_ = MAX_SAFE_CURRENT_AMPS;

        return CoilResult{
            CoilError::OverCurrent,
            currentAmps_
        };
    }

    // Negative current prevention
    if (requestedCurrentAmps < 0.0f)
    {
        currentAmps_ = 0.0f;

        return CoilResult{
            CoilError::None,
            0.0f
        };
    }

    currentAmps_ = requestedCurrentAmps;

    return CoilResult{
        CoilError::None,
        currentAmps_
    };
}


ErrorCode CoilDriver::apply(
    const DampingCommand& command)
{
    if (command.coilCurrentAmps < MIN_SAFE_CURRENT_AMPS)
    {
        currentAmps_ = MIN_SAFE_CURRENT_AMPS;

        return ErrorCode::COIL_INVALID_NEGATIVE_CURRENT;
    }

    if (command.coilCurrentAmps > MAX_SAFE_CURRENT_AMPS)
    {
        currentAmps_ = MAX_SAFE_CURRENT_AMPS;

        return ErrorCode::COIL_OVERCURRENT_CLAMPED;
    }

    currentAmps_ = command.coilCurrentAmps;

    return ErrorCode::SUCCESS;
}


float CoilDriver::current() const noexcept
{
    return currentAmps_;
}


/* ============================================================
 * QEMU OVER-CURRENT TEST
 *
 * Deliberately requests 6 A.
 *
 * Safe limit = 5 A
 *
 * Expected:
 *
 * requested = 6 A
 *       ↓
 * CoilDriver
 *       ↓
 * OverCurrent
 *       ↓
 * actual current = 5 A
 * ============================================================ */

CoilResult CoilDriver::injectOverCurrentTest() noexcept
{
    constexpr float TEST_CURRENT_AMPS = 6.0f;

    return setCurrent(
        TEST_CURRENT_AMPS,
        25.0f
    );
}
