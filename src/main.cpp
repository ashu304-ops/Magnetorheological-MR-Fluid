
extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

#include <memory>
#include <string>

#include "SuspensionController.hpp"
#include "SensorReader.hpp"
#include "CoilDriver.hpp"
#include "TelemetryLogger.hpp"
#include "TelemetryFormatter.hpp"
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
static TelemetryLogger* g_logger = nullptr;
static SensorReader* g_sensor = nullptr;


/* ============================================================
 * CONTROL TASK
 *
 * 100 Hz
 *
 * Period = 10 ms
 *
 * Responsibilities:
 *
 * Sensor
 *   ↓
 * Filter
 *   ↓
 * Strategy
 *   ↓
 * Coil
 *   ↓
 * Logger
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
        if (g_controller != nullptr)
        {
            g_controller->runCycle(25.0f);
        }

        counter++;

        if ((counter % 100U) == 0U)
        {
            hw_uart_puts(
                "[TASK] CONTROL: 100 cycles completed\r\n"
            );
        }

        /*
         * 100 Hz control loop.
         */
        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}


/* ============================================================
 * TELEMETRY TASK
 *
 * Runs every 1 second.
 *
 * Reads the latest telemetry record from the logger
 * and sends it through UART.
 * ============================================================ */

static void telemetry_task(void* argument)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] TELEMETRY task started\r\n"
    );

    for (;;)
    {
        if (g_logger != nullptr)
        {
            TelemetryRecord record =
                g_logger->getLatest();

            std::string message =
                TelemetryFormatter::format(record);

            hw_uart_puts(
                message.c_str()
            );
        }

        /*
         * Telemetry rate = 1 Hz.
         */
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
 *   0 sec
 *      Normal operation
 *
 *   3 sec
 *      Inject sensor timeout
 *
 *   After 3 sec
 *      Sensor continuously reports timeout
 *      Controller enters safe mode
 *      Coil is commanded to 0 A
 * ============================================================ */

static void sensor_fault_test_task(void* argument)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] SENSOR TEST task started\r\n"
    );

    /*
     * Allow the system to run normally for 3 seconds.
     */
    vTaskDelay(
        pdMS_TO_TICKS(3000)
    );

    if (g_sensor != nullptr)
    {
        hw_uart_puts(
            "[TEST] Injecting SENSOR TIMEOUT...\r\n"
        );

        g_sensor->injectTimeout(true);
    }

    /*
     * Keep the injected fault active.
     */
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
     * Hardware initialization
     * ======================================================== */

    hw_init();

    hw_gpio_init();

    hw_uart_puts(
        "[MAIN] Hardware initialized\r\n"
    );


    /* ========================================================
     * Application objects
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
     * Select damping strategy
     * ======================================================== */

    controller.setStrategy(
        std::make_unique<ComfortStrategy>()
    );


    /* ========================================================
     * Store global references
     * ======================================================== */

    g_controller = &controller;
    g_logger = &logger;
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
     * START FREERTOS
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Starting FreeRTOS scheduler...\r\n"
    );

    vTaskStartScheduler();


    /* ========================================================
     * Scheduler should never return
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] ERROR: Scheduler returned!\r\n"
    );

    for (;;)
    {
        __asm volatile ("nop");
    }
}
