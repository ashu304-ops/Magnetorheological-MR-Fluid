#include "MqttClient.hpp"

extern "C" void hw_uart_puts(const char* str);


/* ============================================================
 * CONNECT
 * ============================================================ */

bool MqttClient::connect() noexcept
{
    /*
     * Temporary QEMU implementation.
     *
     * Later this will perform the actual
     * MQTT/TCP connection.
     */

    hw_uart_puts(
        "[MQTT] Connecting to broker...\r\n"
    );

    connected_ = true;

    hw_uart_puts(
        "[MQTT] Broker connected\r\n"
    );

    return true;
}


/* ============================================================
 * PUBLISH
 * ============================================================ */

bool MqttClient::publish(
    const char* topic,
    const char* payload
) noexcept
{
    if (!connected_)
    {
        hw_uart_puts(
            "[MQTT] ERROR: Not connected\r\n"
        );

        return false;
    }

    hw_uart_puts(
        "[MQTT] topic="
    );

    hw_uart_puts(topic);

    hw_uart_puts(
        " payload="
    );

    hw_uart_puts(payload);

    hw_uart_puts(
        "\r\n"
    );

    return true;
}


/* ============================================================
 * CONNECTION STATUS
 * ============================================================ */

bool MqttClient::isConnected() const noexcept
{
    return connected_;
}