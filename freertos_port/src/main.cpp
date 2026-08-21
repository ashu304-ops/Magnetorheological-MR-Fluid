
extern "C"
{
#include "FreeRTOS.h"
#include "task.h"
}

#include <memory>
#include <stdint.h>

#include "SuspensionController.hpp"
#include "SensorReader.hpp"
#include "CoilDriver.hpp"
#include "TelemetryLogger.hpp"
#include "TelemetryPublisher.hpp"
#include "TelemetryData.hpp"
#include "MqttClient.hpp"
#include "ComfortStrategy.hpp"


/* ============================================================
 * HARDWARE FUNCTIONS
 * ============================================================ */

extern "C" void hw_init(void);
extern "C" void hw_gpio_init(void);
extern "C" void hw_uart_puts(const char* str);


/* ============================================================
 * GLOBAL APPLICATION REFERENCES
 * ============================================================ */

static SuspensionController* g_controller = nullptr;
static SensorReader* g_sensor = nullptr;
static CoilDriver* g_coil = nullptr;


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
 * FLOAT FORMAT - 3 DECIMAL PLACES
 * ============================================================ */

static void format3(
    float value,
    char* buffer
)
{
    int32_t scaled;

    if (value >= 0.0f)
    {
        scaled = static_cast<int32_t>(
            value * 1000.0f + 0.5f
        );
    }
    else
    {
        scaled = static_cast<int32_t>(
            value * 1000.0f - 0.5f
        );
    }

    char* p = buffer;

    if (scaled < 0)
    {
        *p++ = '-';
        scaled = -scaled;
    }

    const int32_t integerPart =
        scaled / 1000;

    const int32_t decimalPart =
        scaled % 1000;

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
        '0' + (decimalPart / 100) % 10
    );

    *p++ = static_cast<char>(
        '0' + (decimalPart / 10) % 10
    );

    *p++ = static_cast<char>(
        '0' + decimalPart % 10
    );

    *p = '\0';
}


/* ============================================================
 * SENSOR ERROR -> STRING
 * ============================================================ */

static const char* sensorErrorToString(
    SensorError error
)
{
    switch (error)
    {
        case SensorError::None:
            return "NONE";

        case SensorError::Timeout:
            return "TIMEOUT";

        case SensorError::ReadTimeout:
            return "READ_TIMEOUT";

        case SensorError::InvalidValue:
            return "INVALID_VALUE";

        case SensorError::OutOfRange:
            return "OUT_OF_RANGE";

        case SensorError::HardwareFault:
            return "HARDWARE_FAULT";

        default:
            return "UNKNOWN";
    }
}


/* ============================================================
 * COIL ERROR -> STRING
 * ============================================================ */

static const char* coilErrorToString(
    CoilError error
)
{
    switch (error)
    {
        case CoilError::None:
            return "NONE";

        case CoilError::OverCurrent:
            return "OVER_CURRENT";

        case CoilError::OverTemperature:
            return "OVER_TEMPERATURE";

        case CoilError::HardwareFault:
            return "HARDWARE_FAULT";

        default:
            return "UNKNOWN";
    }
}


/* ============================================================
 * LOCAL UART TELEMETRY
 *
 * Keeps the QEMU output useful for debugging.
 * ============================================================ */

static void printTelemetry()
{
    if (g_controller == nullptr)
    {
        return;
    }

    char acceleration[16];
    char force[16];
    char current[16];
    char temperature[16];

    format2(
        g_controller->lastAccelerationG(),
        acceleration
    );

    format2(
        g_controller->lastForceN(),
        force
    );

    format3(
        g_controller->lastRequestedCurrentA(),
        current
    );

    format2(
        g_controller->lastTemperatureC(),
        temperature
    );

    hw_uart_puts("[TEL] acc=");
    hw_uart_puts(acceleration);

    hw_uart_puts("g force=");
    hw_uart_puts(force);

    hw_uart_puts("N current=");
    hw_uart_puts(current);

    hw_uart_puts("A temp=");
    hw_uart_puts(temperature);

    hw_uart_puts("C\r\n");


    if (g_controller->isSafeMode())
    {
        hw_uart_puts(
            "[TEL] WARNING: SAFE MODE ACTIVE\r\n"
        );

        hw_uart_puts(
            "[TEL] SensorError="
        );

        hw_uart_puts(
            sensorErrorToString(
                g_controller->lastSensorError()
            )
        );

        hw_uart_puts("\r\n");

        hw_uart_puts(
            "[TEL] CoilError="
        );

        hw_uart_puts(
            coilErrorToString(
                g_controller->lastCoilError()
            )
        );

        hw_uart_puts("\r\n");
    }
}


/* ============================================================
 * CONTROL TASK
 *
 * 100 Hz
 * 10 ms period
 * ============================================================ */

static void control_task(
    void* argument
)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] CONTROL task started\r\n"
    );

    uint32_t counter = 0U;

    for (;;)
    {
        if (g_controller != nullptr)
        {
            g_controller->runCycle(25.0f);
        }

        ++counter;

        if ((counter % 100U) == 0U)
        {
            hw_uart_puts(
                "[TASK] CONTROL: 100 cycles completed\r\n"
            );
        }

        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}


/* ============================================================
 * TELEMETRY TASK
 *
 * 1 Hz
 *
 * Controller
 *      ↓
 * TelemetryData
 *      ↓
 * TelemetryPublisher
 *      ↓
 * MqttClient
 *      ↓
 * MQTT transport
 * ============================================================ */

static void telemetry_task(
    void* argument
)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] TELEMETRY task started\r\n"
    );


    /* --------------------------------------------------------
     * MQTT client.
     *
     * Static because this is an embedded application.
     * -------------------------------------------------------- */

    static MqttClient mqttClient;


    /* --------------------------------------------------------
     * Telemetry publisher uses dependency injection.
     * -------------------------------------------------------- */

    static TelemetryPublisher publisher(
        mqttClient
    );


    /* --------------------------------------------------------
     * Connect once before publishing.
     * -------------------------------------------------------- */

    hw_uart_puts(
        "[MQTT] Initializing publisher...\r\n"
    );

    if (!mqttClient.connect())
    {
        hw_uart_puts(
            "[MQTT] ERROR: Initial connection failed\r\n"
        );
    }
    else
    {
        hw_uart_puts(
            "[MQTT] Publisher ready\r\n"
        );
    }


    /* ========================================================
     * TELEMETRY LOOP
     * ======================================================== */

    for (;;)
    {
        if (g_controller != nullptr)
        {
            /* =================================================
             * CREATE TELEMETRY DATA
             * ================================================= */

            TelemetryData data{};

            data.accelerationG =
                g_controller->lastAccelerationG();

            data.forceN =
                g_controller->lastForceN();

            data.currentA =
                g_controller->lastRequestedCurrentA();

            data.temperatureC =
                g_controller->lastTemperatureC();

            data.safeMode =
                g_controller->isSafeMode();

            data.sensorError =
                static_cast<uint8_t>(
                    g_controller->lastSensorError()
                );

            data.coilError =
                static_cast<uint8_t>(
                    g_controller->lastCoilError()
                );


            /* =================================================
             * TIMESTAMP
             *
             * FreeRTOS tick → milliseconds
             * ================================================= */

            data.timestampMs =
                static_cast<uint32_t>(
                    xTaskGetTickCount()
                    *
                    portTICK_PERIOD_MS
                );


            /* =================================================
             * LOCAL DEBUG OUTPUT
             * ================================================= */

            printTelemetry();


            /* =================================================
             * MQTT PUBLISH
             * ================================================= */

            const bool published =
                publisher.publish(data);

            if (!published)
            {
                hw_uart_puts(
                    "[MQTT] ERROR: Telemetry publish failed\r\n"
                );
            }
        }


        /* ----------------------------------------------------
         * 1 Hz telemetry
         * ---------------------------------------------------- */

        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}


/* ============================================================
 * SENSOR + COIL FAULT TEST TASK
 *
 * QEMU ONLY
 * ============================================================ */

static void fault_test_task(
    void* argument
)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] FAULT TEST task started\r\n"
    );


    /* ========================================================
     * NORMAL OPERATION
     * ======================================================== */

    hw_uart_puts(
        "[TEST] NORMAL OPERATION - 3 seconds\r\n"
    );

    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );


    /* ========================================================
     * SENSOR TIMEOUT
     * ======================================================== */

    if (g_sensor != nullptr)
    {
        hw_uart_puts(
            "[TEST] Injecting SENSOR TIMEOUT...\r\n"
        );

        g_sensor->injectTimeout(true);
    }


    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );


    /* ========================================================
     * SENSOR RECOVERY
     * ======================================================== */

    if (g_sensor != nullptr)
    {
        hw_uart_puts(
            "[TEST] Recovering SENSOR...\r\n"
        );

        g_sensor->injectTimeout(false);
    }


    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );


    /* ========================================================
     * OVER-CURRENT TEST
     * ======================================================== */

    if (g_coil != nullptr)
    {
        hw_uart_puts(
            "[TEST] Injecting OVER-CURRENT...\r\n"
        );

        const CoilResult result =
            g_coil->injectOverCurrentTest();


        if (result.error ==
            CoilError::OverCurrent)
        {
            hw_uart_puts(
                "[TEST] OVER-CURRENT DETECTED\r\n"
            );

            if (g_controller != nullptr)
            {
                g_controller->enterSafeMode(
                    result.error
                );
            }
        }
        else
        {
            hw_uart_puts(
                "[TEST] ERROR: OVER-CURRENT TEST FAILED\r\n"
            );
        }
    }


    /* ========================================================
     * FAULT TEST COMPLETE
     * ======================================================== */

    hw_uart_puts(
        "[TEST] FAULT TEST COMPLETE\r\n"
    );


    /* ========================================================
     * WAIT BEFORE RECOVERY
     * ======================================================== */

    hw_uart_puts(
        "[TEST] Waiting before COIL fault recovery...\r\n"
    );

    vTaskDelay(
        pdMS_TO_TICKS(2000)
    );


    /* ========================================================
     * CLEAR COIL FAULT
     * ======================================================== */

    hw_uart_puts(
        "[TEST] Clearing COIL fault...\r\n"
    );

    if (g_controller != nullptr)
    {
        g_controller->clearCoilFault();
    }


    /* ========================================================
     * RECOVERY STABILIZATION
     * ======================================================== */

    vTaskDelay(
        pdMS_TO_TICKS(1000)
    );


    hw_uart_puts(
        "[TEST] COIL fault recovery complete\r\n"
    );


    /* ========================================================
     * KEEP TASK ALIVE
     * ======================================================== */

    for (;;)
    {
        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}


/* ============================================================
 * MAIN
 * ============================================================ */

extern "C" int main(void)
{
    /* ========================================================
     * HARDWARE INITIALIZATION
     * ======================================================== */

    hw_init();

    hw_gpio_init();


    hw_uart_puts(
        "\r\n==============================\r\n"
    );

    hw_uart_puts(
        "STM32F4 QEMU START\r\n"
    );

    hw_uart_puts(
        "==============================\r\n"
    );

    hw_uart_puts(
        "[MAIN] Hardware initialized\r\n"
    );


    /* ========================================================
     * APPLICATION OBJECTS
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Creating application objects\r\n"
    );


    static SensorReader sensor;

    static CoilDriver coil;

    static TelemetryLogger logger;


    static SuspensionController controller(
        sensor,
        coil,
        logger
    );


    /* ========================================================
     * DAMPING STRATEGY
     * ======================================================== */

    controller.setStrategy(
        std::make_unique<ComfortStrategy>()
    );


    /* ========================================================
     * GLOBAL REFERENCES
     * ======================================================== */

    g_controller =
        &controller;

    g_sensor =
        &sensor;

    g_coil =
        &coil;


    hw_uart_puts(
        "[MAIN] Controller initialized\r\n"
    );


    /* ========================================================
     * CONTROL TASK
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Creating CONTROL task\r\n"
    );

    BaseType_t result =
        xTaskCreate(
            control_task,
            "CONTROL",
            512,
            nullptr,
            2,
            nullptr
        );

    if (result != pdPASS)
    {
        hw_uart_puts(
            "[MAIN] ERROR: CONTROL task creation failed\r\n"
        );

        for (;;)
        {
            __asm volatile ("nop");
        }
    }

    hw_uart_puts(
        "[MAIN] CONTROL task created successfully\r\n"
    );


    /* ========================================================
     * TELEMETRY TASK
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Creating TELEMETRY task\r\n"
    );

    result =
        xTaskCreate(
            telemetry_task,
            "TELEMETRY",
            768,
            nullptr,
            1,
            nullptr
        );

    if (result != pdPASS)
    {
        hw_uart_puts(
            "[MAIN] ERROR: TELEMETRY task creation failed\r\n"
        );

        for (;;)
        {
            __asm volatile ("nop");
        }
    }

    hw_uart_puts(
        "[MAIN] TELEMETRY task created successfully\r\n"
    );


    /* ========================================================
     * FAULT TEST TASK
     *
     * QEMU ONLY
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Creating FAULT TEST task\r\n"
    );

    result =
        xTaskCreate(
            fault_test_task,
            "FAULT_TEST",
            512,
            nullptr,
            1,
            nullptr
        );

    if (result != pdPASS)
    {
        hw_uart_puts(
            "[MAIN] ERROR: FAULT TEST task creation failed\r\n"
        );

        for (;;)
        {
            __asm volatile ("nop");
        }
    }

    hw_uart_puts(
        "[MAIN] FAULT TEST task created successfully\r\n"
    );


    /* ========================================================
     * START FREERTOS
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Starting FreeRTOS scheduler...\r\n"
    );

    vTaskStartScheduler();


    /* ========================================================
     * SHOULD NEVER REACH HERE
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] ERROR: Scheduler returned!\r\n"
    );

    for (;;)
    {
        __asm volatile ("nop");
    }
}
