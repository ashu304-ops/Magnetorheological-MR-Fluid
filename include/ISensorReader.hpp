#pragma once

#include "ErrorTypes.hpp"


class ISensorReader
{
public:

    virtual ~ISensorReader() = default;


    virtual SensorReadResult read() noexcept = 0;
};