#include "SensorReader.hpp"

#include <cstdint>


SensorReader::SensorReader(
    float initialG
) noexcept
    : currentG_(initialG)
{
}


/* ============================================================
 * READ SENSOR
 * ============================================================ */

SensorReadResult SensorReader::read() noexcept
{
    SensorReadResult result{};

    /* --------------------------------------------------------
     * Simulate communication timeout.
     * -------------------------------------------------------- */

    if (communicationTimedOut())
    {
        result.error =
            SensorError::Timeout;

        result.accelerationG =
            0.0f;

        return result;
    }


    /* --------------------------------------------------------
     * Read simulated hardware.
     * -------------------------------------------------------- */

    const float value =
        readHardware();


    result.error =
        SensorError::None;

    result.accelerationG =
        value;

    return result;
}


/* ============================================================
 * SIMULATED SENSOR HARDWARE
 * ============================================================ */

float SensorReader::readHardware() const noexcept
{
    /*
     * If QEMU test injection is active,
     * return the injected value.
     */

    if (injectedReading_)
    {
        return currentG_;
    }


    /*
     * Otherwise generate deterministic
     * suspension vibration.
     */

    static uint32_t sample = 0U;

    ++sample;

    const uint32_t phase =
        sample % 40U;

    float value = 0.0f;


    if (phase < 10U)
    {
        value =
            static_cast<float>(phase) * 0.08f;
    }
    else if (phase < 20U)
    {
        value =
            static_cast<float>(20U - phase) * 0.08f;
    }
    else if (phase < 30U)
    {
        value =
            -static_cast<float>(phase - 20U) * 0.08f;
    }
    else
    {
        value =
            -static_cast<float>(40U - phase) * 0.08f;
    }


    return value;
}


/* ============================================================
 * COMMUNICATION TIMEOUT
 * ============================================================ */

bool SensorReader::communicationTimedOut() const noexcept
{
    return timedOut_;
}


/* ============================================================
 * INJECT SENSOR VALUE
 * ============================================================ */

void SensorReader::injectHardwareReading(
    float gForce
) noexcept
{
    currentG_ =
        gForce;

    injectedReading_ =
        true;
}


/* ============================================================
 * CLEAR SENSOR VALUE INJECTION
 * ============================================================ */

void SensorReader::clearHardwareInjection() noexcept
{
    injectedReading_ =
        false;
}


/* ============================================================
 * INJECT TIMEOUT
 * ============================================================ */

void SensorReader::injectTimeout(
    bool timedOut
) noexcept
{
    timedOut_ =
        timedOut;
}