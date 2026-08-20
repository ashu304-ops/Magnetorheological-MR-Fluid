extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

#include <memory>
#include <stdint.h>

#include "SuspensionController.hpp"
#include "SensorReader.hpp"
#include "CoilDriver.hpp"
#include "TelemetryLogger.hpp"
#include "ComfortStrategy.hpp"


/* ============================================================
 * Hardware functions
 * ============================================================ */

extern "C" void hw_init(void);
extern "C" void hw_gpio_init(void);
extern "C" void hw_uart_puts(const char* str);


/* ============================================================
 * Global objects
 * ============================================================ */

static SuspensionController* g_controller = nullptr;
static SensorReader* g_sensor = nullptr;


/* ============================================================
 * Embedded-friendly number formatter
 *
 * Example:
 *     0.08  -> "0.08"
 *    -4.00  -> "-4.00"
 * ============================================================ */

static void format2(float value, char* buffer)
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

    const int32_t integerPart = scaled / 100;
    const int32_t decimalPart = scaled % 100;

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
 * 3 decimal places
 *
 * Example:
 *     0.040 -> "0.040"
 * ============================================================ */

static void format3(float value, char* buffer)
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

    const int32_t integerPart = scaled / 1000;
    const int32_t decimalPart = scaled % 1000;

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
 * SENSOR ERROR STRING
 * ============================================================ */

static const char* sensorErrorToString(
    SensorError error)
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
 * COIL ERROR STRING
 * ============================================================ */

static const char* coilErrorToString(
    CoilError error)
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
 * TELEMETRY OUTPUT
 *
 * Runs from TELEMETRY task only.
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


    /* --------------------------------------------------------
     * Main telemetry
     * -------------------------------------------------------- */

    hw_uart_puts("[TEL] acc=");
    hw_uart_puts(acceleration);

    hw_uart_puts("g force=");
    hw_uart_puts(force);

    hw_uart_puts("N current=");
    hw_uart_puts(current);

    hw_uart_puts("A\r\n");


    /* --------------------------------------------------------
     * Safe mode information
     * -------------------------------------------------------- */

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
 * Frequency:
 *     100 Hz
 *
 * Period:
 *     10 ms
 *
 * Priority:
 *     2
 * ============================================================ */

static void control_task(void* argument)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] CONTROL task started\r\n"
    );

    uint32_t counter = 0U;

    for (;;)
    {
        /* ----------------------------------------------------
         * Execute control cycle
         * ---------------------------------------------------- */

        if (g_controller != nullptr)
        {
            g_controller->runCycle(25.0f);
        }


        /* ----------------------------------------------------
         * Debug message once per second
         * ---------------------------------------------------- */

        counter++;

        if ((counter % 100U) == 0U)
        {
            hw_uart_puts(
                "[TASK] CONTROL: 100 cycles completed\r\n"
            );
        }


        /* ----------------------------------------------------
         * 100 Hz
         * ---------------------------------------------------- */

        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}


/* ============================================================
 * TELEMETRY TASK
 *
 * Frequency:
 *     1 Hz
 *
 * Priority:
 *     1
 *
 * CONTROL task does not perform telemetry output.
 * ============================================================ */

static void telemetry_task(void* argument)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] TELEMETRY task started\r\n"
    );

    for (;;)
    {
        printTelemetry();

        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}


/* ============================================================
 * SENSOR FAULT TEST TASK
 *
 * QEMU TEST ONLY
 *
 * Timeline:
 *
 * 0 - 3 sec
 *     Normal operation
 *
 * 3 sec
 *     Inject sensor timeout
 *
 * 3 - 6 sec
 *     Sensor remains failed
 *
 * 6 sec
 *     Sensor recovery
 *
 * Expected:
 *
 * Normal
 *   ↓
 * Sensor Timeout
 *   ↓
 * Safe Mode
 *   ↓
 * Coil OFF
 *   ↓
 * Sensor Recovery
 *   ↓
 * Normal Operation
 * ============================================================ */

static void sensor_fault_test_task(void* argument)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] SENSOR TEST task started\r\n"
    );


    /* --------------------------------------------------------
     * Allow normal operation for 3 seconds
     * -------------------------------------------------------- */

    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );


    /* --------------------------------------------------------
     * Inject sensor timeout
     * -------------------------------------------------------- */

    if (g_sensor != nullptr)
    {
        hw_uart_puts(
            "[TEST] Injecting SENSOR TIMEOUT...\r\n"
        );

        g_sensor->injectTimeout(true);
    }


    /* --------------------------------------------------------
     * Keep fault active for 3 seconds
     * -------------------------------------------------------- */

    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );


    /* --------------------------------------------------------
     * Recover sensor
     * -------------------------------------------------------- */

    if (g_sensor != nullptr)
    {
        hw_uart_puts(
            "[TEST] Recovering SENSOR...\r\n"
        );

        g_sensor->injectTimeout(false);
    }


    /* --------------------------------------------------------
     * Test task remains alive
     * -------------------------------------------------------- */

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
     * SELECT CONTROL STRATEGY
     * ======================================================== */

    controller.setStrategy(
        std::make_unique<ComfortStrategy>()
    );


    /* ========================================================
     * STORE GLOBAL REFERENCES
     * ======================================================== */

    g_controller = &controller;

    g_sensor = &sensor;


    hw_uart_puts(
        "[MAIN] Controller initialized\r\n"
    );


    /* ========================================================
     * CREATE CONTROL TASK
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Creating CONTROL task\r\n"
    );

    BaseType_t result = xTaskCreate(
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
     * CREATE TELEMETRY TASK
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Creating TELEMETRY task\r\n"
    );

    result = xTaskCreate(
        telemetry_task,
        "TELEMETRY",
        512,
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
     * CREATE SENSOR TEST TASK
     *
     * QEMU ONLY
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Creating SENSOR TEST task\r\n"
    );

    result = xTaskCreate(
        sensor_fault_test_task,
        "SENSOR_TEST",
        256,
        nullptr,
        1,
        nullptr
    );


    if (result != pdPASS)
    {
        hw_uart_puts(
            "[MAIN] ERROR: SENSOR TEST task creation failed\r\n"
        );

        for (;;)
        {
            __asm volatile ("nop");
        }
    }


    hw_uart_puts(
        "[MAIN] SENSOR TEST task created successfully\r\n"
    );


    /* ========================================================
     * START FREERTOS SCHEDULER
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Starting FreeRTOS scheduler...\r\n"
    );

    vTaskStartScheduler();


    /* ========================================================
     * SCHEDULER SHOULD NEVER RETURN
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] ERROR: Scheduler returned!\r\n"
    );


    for (;;)
    {
        __asm volatile ("nop");
    }
}