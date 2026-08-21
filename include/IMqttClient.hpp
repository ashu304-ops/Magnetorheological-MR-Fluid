#ifndef I_MQTT_CLIENT_HPP
#define I_MQTT_CLIENT_HPP

class IMqttClient
{
public:

    virtual ~IMqttClient() = default;

    virtual bool connect() noexcept = 0;

    virtual bool publish(
        const char* topic,
        const char* payload
    ) noexcept = 0;

    virtual bool isConnected() const noexcept = 0;
};

#endif