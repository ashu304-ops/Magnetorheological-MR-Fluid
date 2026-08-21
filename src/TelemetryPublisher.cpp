#include "TelemetryPublisher.hpp"

#include <stdint.h>

static constexpr const char* MQTT_TOPIC =
    "vehicle/suspension/telemetry";


/* ============================================================
 * FLOAT FORMAT - 2 DECIMAL PLACES
 * ============================================================ */

static void format2(
    float value,
    char* buffer
)
{
    int32_t scaled;

    if (value >= 0.0f)
    {
        scaled = static_cast<int32_t>(
            value * 100.0f + 0.5f
        );
    }
    else
    {
        scaled = static_cast<int32_t>(
            value * 100.0f - 0.5f
        );
    }

    char* p = buffer;

    if (scaled < 0)
    {
        *p++ = '-';
        scaled = -scaled;
    }

    const int32_t integerPart =
        scaled / 100;

    const int32_t decimalPart =
        scaled % 100;

    if (integerPart >= 100)
    {
        *p++ = static_cast<char>(
            '0' + (integerPart / 100) % 10
        );
    }

    if (integerPart >= 10)
    {
        *p++ = static_cast<char>(
            '0' + (integerPart / 10) % 10
        );
    }

    *p++ = static_cast<char>(
        '0' + integerPart % 10
    );

    *p++ = '.';

    *p++ = static_cast<char>(
        '0' + decimalPart / 10
    );

    *p++ = static_cast<char>(
        '0' + decimalPart % 10
    );

    *p = '\0';
}


/* ============================================================
 * UINT32 FORMAT
 * ============================================================ */

static void formatUint32(
    uint32_t value,
    char* buffer
)
{
    char temp[11];

    int index = 0;

    do
    {
        temp[index++] =
            static_cast<char>(
                '0' + (value % 10U)
            );

        value /= 10U;

    } while (value > 0U);


    int outputIndex = 0;

    while (index > 0)
    {
        buffer[outputIndex++] =
            temp[--index];
    }

    buffer[outputIndex] = '\0';
}


/* ============================================================
 * CONSTRUCTOR
 * ============================================================ */

TelemetryPublisher::TelemetryPublisher(
    IMqttClient& mqttClient
) noexcept
    : mqttClient_(mqttClient)
{
}


/* ============================================================
 * PUBLISH TELEMETRY
 * ============================================================ */

bool TelemetryPublisher::publish(
    const TelemetryData& data
) noexcept
{
    char acceleration[16];
    char force[16];
    char current[16];
    char temperature[16];
    char timestamp[16];

    format2(
        data.accelerationG,
        acceleration
    );

    format2(
        data.forceN,
        force
    );

    format2(
        data.currentA,
        current
    );

    format2(
        data.temperatureC,
        temperature
    );

    formatUint32(
        data.timestampMs,
        timestamp
    );


    /* ========================================================
     * BUILD JSON PAYLOAD
     * ======================================================== */

    char payload[256];

    char* p = payload;


    /* --------------------------------------------------------
     * Beginning
     * -------------------------------------------------------- */

    *p++ = '{';


    /* acceleration */

    *p++ = '"';
    *p++ = 'a';
    *p++ = 'c';
    *p++ = 'c';
    *p++ = 'e';
    *p++ = 'l';
    *p++ = 'e';
    *p++ = 'r';
    *p++ = 'a';
    *p++ = 't';
    *p++ = 'i';
    *p++ = 'o';
    *p++ = 'n';
    *p++ = '"';
    *p++ = ':';

    for (const char* c = acceleration; *c != '\0'; ++c)
    {
        *p++ = *c;
    }


    /* force */

    *p++ = ',';
    *p++ = '"';
    *p++ = 'f';
    *p++ = 'o';
    *p++ = 'r';
    *p++ = 'c';
    *p++ = 'e';
    *p++ = '"';
    *p++ = ':';

    for (const char* c = force; *c != '\0'; ++c)
    {
        *p++ = *c;
    }


    /* current */

    *p++ = ',';
    *p++ = '"';
    *p++ = 'c';
    *p++ = 'u';
    *p++ = 'r';
    *p++ = 'r';
    *p++ = 'e';
    *p++ = 'n';
    *p++ = 't';
    *p++ = '"';
    *p++ = ':';

    for (const char* c = current; *c != '\0'; ++c)
    {
        *p++ = *c;
    }


    /* temperature */

    *p++ = ',';
    *p++ = '"';
    *p++ = 't';
    *p++ = 'e';
    *p++ = 'm';
    *p++ = 'p';
    *p++ = 'e';
    *p++ = 'r';
    *p++ = 'a';
    *p++ = 't';
    *p++ = 'u';
    *p++ = 'r';
    *p++ = 'e';
    *p++ = '"';
    *p++ = ':';

    for (const char* c = temperature; *c != '\0'; ++c)
    {
        *p++ = *c;
    }


    /* safeMode */

    const char* safeMode =
        data.safeMode ? "true" : "false";

    *p++ = ',';
    *p++ = '"';
    *p++ = 's';
    *p++ = 'a';
    *p++ = 'f';
    *p++ = 'e';
    *p++ = 'M';
    *p++ = 'o';
    *p++ = 'd';
    *p++ = 'e';
    *p++ = '"';
    *p++ = ':';

    for (const char* c = safeMode; *c != '\0'; ++c)
    {
        *p++ = *c;
    }


    /* sensorError */

    *p++ = ',';
    *p++ = '"';
    *p++ = 's';
    *p++ = 'e';
    *p++ = 'n';
    *p++ = 's';
    *p++ = 'o';
    *p++ = 'r';
    *p++ = 'E';
    *p++ = 'r';
    *p++ = 'r';
    *p++ = 'o';
    *p++ = 'r';
    *p++ = '"';
    *p++ = ':';

    if (data.sensorError == 0U)
    {
        *p++ = '0';
    }
    else
    {
        *p++ = '1';
    }


    /* coilError */

    *p++ = ',';
    *p++ = '"';
    *p++ = 'c';
    *p++ = 'o';
    *p++ = 'i';
    *p++ = 'l';
    *p++ = 'E';
    *p++ = 'r';
    *p++ = 'r';
    *p++ = 'o';
    *p++ = 'r';
    *p++ = '"';
    *p++ = ':';

    if (data.coilError == 0U)
    {
        *p++ = '0';
    }
    else
    {
        *p++ = '1';
    }


    /* timestamp */

    *p++ = ',';
    *p++ = '"';
    *p++ = 't';
    *p++ = 'i';
    *p++ = 'm';
    *p++ = 'e';
    *p++ = 's';
    *p++ = 't';
    *p++ = 'a';
    *p++ = 'm';
    *p++ = 'p';
    *p++ = '"';
    *p++ = ':';

    for (const char* c = timestamp; *c != '\0'; ++c)
    {
        *p++ = *c;
    }


    /* End JSON */

    *p++ = '}';

    *p = '\0';


    /* ========================================================
     * ACTUAL MQTT PUBLISH
     * ======================================================== */

    if (!mqttClient_.isConnected())
    {
        return false;
    }

    return mqttClient_.publish(
        MQTT_TOPIC,
        payload
    );
}