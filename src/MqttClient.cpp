
#include "MqttClient.hpp"

extern "C" void hw_uart_puts(const char* str);


/* ============================================================
 * CONNECT
 *
 * Option A:
 * QEMU does not directly implement TCP/MQTT.
 *
 * The Linux host reads the telemetry line from QEMU stdout
 * and forwards it to Mosquitto.
 * ============================================================ */

bool MqttClient::connect() noexcept
{
    hw_uart_puts(
        "[MQTT] Connecting to host MQTT bridge...\r\n"
    );

    /*
     * The host bridge is responsible for the real
     * MQTT connection to Mosquitto :1883.
     *
     * From the firmware point of view, the transport
     * is considered ready once the bridge is available.
     */

    connected_ = true;

    hw_uart_puts(
        "[MQTT] Host MQTT bridge ready\r\n"
    );

    return true;
}


/* ============================================================
 * PUBLISH
 *
 * Format:
 *
 * [MQTT-PUBLISH] topic=... payload=...
 *
 * The Linux MQTT bridge will detect this line and publish
 * the payload to Mosquitto.
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
        "[MQTT-PUBLISH] topic="
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