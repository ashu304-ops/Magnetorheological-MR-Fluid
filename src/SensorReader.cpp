#include "SensorReader.hpp"
#include <cstdint>

SensorReader::SensorReader(float initialG) noexcept
    : currentG_(initialG)
{
}


/* ============================================================
 * Read sensor
 * ============================================================ */

SensorReadResult SensorReader::read() noexcept
{
    SensorReadResult result{};

    if (communicationTimedOut())
    {
        result.error = SensorError::Timeout;
        result.accelerationG = 0.0f;

        return result;
    }

    /*
     * Generate a simulated suspension vibration.
     *
     * The signal changes every control cycle.
     */
    const float value = readHardware();

    result.error = SensorError::None;

    result.accelerationG = value;

    return result;
}


/* ============================================================
 * Simulated hardware sensor
 *
 * This is NOT real hardware.
 *
 * It generates a repeating suspension-like waveform.
 * ============================================================ */

float SensorReader::readHardware() const noexcept
{
    /*
     * Simple deterministic waveform.
     *
     * No <cmath> required.
     *
     * The value changes between approximately:
     *
     * -0.8 g
     * +0.8 g
     */

    static uint32_t sample = 0;

    sample++;

    const uint32_t phase = sample % 40U;

    float value;

    if (phase < 10U)
    {
        value = static_cast<float>(phase) * 0.08f;
    }
    else if (phase < 20U)
    {
        value = static_cast<float>(20U - phase) * 0.08f;
    }
    else if (phase < 30U)
    {
        value = -static_cast<float>(phase - 20U) * 0.08f;
    }
    else
    {
        value = -static_cast<float>(40U - phase) * 0.08f;
    }

    return value;
}


/* ============================================================
 * Communication timeout simulation
 * ============================================================ */

bool SensorReader::communicationTimedOut() const noexcept
{
    return timedOut_;
}


/* ============================================================
 * Test / simulation injection
 * ============================================================ */

void SensorReader::injectHardwareReading(
    float gForce) noexcept
{
    currentG_ = gForce;
}


void SensorReader::injectTimeout(
    bool timedOut) noexcept
{
    timedOut_ = timedOut;
}