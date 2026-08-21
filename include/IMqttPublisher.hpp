#ifndef I_MQTT_PUBLISHER_HPP
#define I_MQTT_PUBLISHER_HPP

#include "TelemetryData.hpp"

class IMqttPublisher
{
public:

    virtual ~IMqttPublisher() = default;

    virtual bool publish(
        const TelemetryData& data
    ) noexcept = 0;
};

#endif