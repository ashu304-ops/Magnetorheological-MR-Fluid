#pragma once

#include "ErrorTypes.hpp"
#include "DampingCommand.hpp"


class ICoilDriver
{
public:

    virtual ~ICoilDriver() = default;


    virtual CoilResult setCurrent(
        float requestedCurrentAmps,
        float temperatureCelsius
    ) noexcept = 0;


    virtual ErrorCode apply(
        const DampingCommand& command
    ) = 0;


    virtual float current() const noexcept = 0;
};