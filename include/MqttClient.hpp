#ifndef MQTT_CLIENT_HPP
#define MQTT_CLIENT_HPP

#include "IMqttClient.hpp"

class MqttClient : public IMqttClient
{
public:

    MqttClient() noexcept = default;

    bool connect() noexcept override;

    bool publish(
        const char* topic,
        const char* payload
    ) noexcept override;

    bool isConnected() const noexcept override;

private:

    bool connected_{false};
};

#endif