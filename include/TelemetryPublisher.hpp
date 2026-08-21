
#ifndef TELEMETRY_PUBLISHER_HPP
#define TELEMETRY_PUBLISHER_HPP

#include "IMqttPublisher.hpp"
#include "IMqttClient.hpp"

class TelemetryPublisher : public IMqttPublisher
{
public:

    explicit TelemetryPublisher(
        IMqttClient& mqttClient
    ) noexcept;


    bool publish(
        const TelemetryData& data
    ) noexcept override;


private:

    IMqttClient& mqttClient_;
};

#endif
