#pragma once

#include "ICoilDriver.hpp"
#include "DampingCommand.hpp"


class CoilDriver : public ICoilDriver
{
public:

    CoilResult setCurrent(
        float requestedCurrentAmps,
        float temperatureCelsius
    ) noexcept override;


    ErrorCode apply(
        const DampingCommand& command
    ) override;


    float current() const noexcept override;


    /* ========================================================
     * QEMU TEST
     * ======================================================== */

    CoilResult injectOverCurrentTest() noexcept;


private:

    float currentAmps_{0.0f};
};