#pragma once

#include <string>

#include "TelemetryRecord.hpp"


class TelemetryFormatter
{
public:

    static std::string format(
        const TelemetryRecord& record
    );
};