#pragma once

#include "ISensorReader.hpp"

class SensorReader : public ISensorReader
{
public:

    explicit SensorReader(
        float initialG = 0.0f
    ) noexcept;


    SensorReadResult read() noexcept override;


    /* ========================================================
     * QEMU TEST HELPERS
     * ======================================================== */

    void injectHardwareReading(
        float gForce
    ) noexcept;


    void clearHardwareInjection() noexcept;


    void injectTimeout(
        bool timedOut
    ) noexcept;


private:

    float readHardware() const noexcept;

    bool communicationTimedOut() const noexcept;


private:

    float currentG_{0.0f};

    bool injectedReading_{false};

    bool timedOut_{false};
};