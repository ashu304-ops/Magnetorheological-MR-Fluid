
extern "C" {
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
}

#include <memory>

#include "SuspensionController.hpp"
#include "SensorReader.hpp"
#include "CoilDriver.hpp"
#include "TelemetryLogger.hpp"
#include "ComfortStrategy.hpp"
#include "TelemetryQueue.hpp"


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

/*
 * Sensor pointer is used only for fault-injection testing.
 *
 * In a real system the sensor would be connected to
 * hardware such as I2C/SPI.
 */
static SensorReader* g_sensor = nullptr;


/*
 * Telemetry queue.
 *
 * Length = 1.
 *
 * We only care about the newest telemetry sample.
 */
static QueueHandle_t g_telemetryQueue = nullptr;


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
 *
 * Flow:
 *
 * Sensor
 *   ↓
 * Filter
 *   ↓
 * Strategy
 *   ↓
 * Coil
 *   ↓
 * Telemetry
 *   ↓
 * FreeRTOS Queue
 *
 * IMPORTANT:
 *
 * No UART telemetry output is performed here.
 * ============================================================ */

static void control_task(void* argument)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] CONTROL task started\r\n"
    );

    uint32_t counter = 0;

    for (;;)
    {
        /* ----------------------------------------------------
         * Fault injection test
         *
         * 500 cycles × 10 ms = 5 seconds
         *
         * After 5 seconds we simulate a sensor communication
         * timeout.
         * ---------------------------------------------------- */

        if (counter == 500U && g_sensor != nullptr)
        {
            hw_uart_puts(
                "\r\n[TEST] Injecting SENSOR TIMEOUT\r\n"
            );

            g_sensor->injectTimeout(true);
        }


        /* ----------------------------------------------------
         * Run control algorithm
         * ---------------------------------------------------- */

        if (g_controller != nullptr)
        {
            g_controller->runCycle(25.0f);
        }


        /* ----------------------------------------------------
         * Publish latest telemetry
         * ---------------------------------------------------- */

        if (g_controller != nullptr &&
            g_telemetryQueue != nullptr)
        {
            TelemetryMessage message{};

            message.accelerationG =
                g_controller->lastAccelerationG();

            message.forceN =
                g_controller->lastForceN();

            message.currentA =
                g_controller->lastRequestedCurrentA();

            message.temperatureC =
                g_controller->lastTemperatureC();

            message.safeMode =
                g_controller->isSafeMode()
                    ? 1U
                    : 0U;


            /*
             * Queue length = 1.
             *
             * xQueueOverwrite() replaces the old sample.
             *
             * CONTROL task never waits for TELEMETRY task.
             */
            xQueueOverwrite(
                g_telemetryQueue,
                &message
            );
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
         * 100 Hz control loop
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
 * Period:
 *     1000 ms
 *
 * Priority:
 *     1
 *
 * CONTROL produces data.
 * TELEMETRY consumes data.
 *
 * FreeRTOS Queue provides task-to-task communication.
 * ============================================================ */

static void telemetry_task(void* argument)
{
    (void)argument;

    hw_uart_puts(
        "[TASK] TELEMETRY task started\r\n"
    );

    TelemetryMessage message{};

    for (;;)
    {
        /*
         * Receive newest telemetry sample.
         *
         * Since CONTROL uses xQueueOverwrite(), the queue
         * always contains the latest available sample.
         */
        if (xQueueReceive(
                g_telemetryQueue,
                &message,
                0) == pdPASS)
        {
            if (message.safeMode != 0U)
            {
                hw_uart_puts(
                    "[TEL] FAULT: SAFE MODE\r\n"
                );

                hw_uart_puts(
                    "[TEL] Actuator output forced to SAFE state\r\n"
                );
            }
            else
            {
                hw_uart_puts(
                    "[TEL] Telemetry received\r\n"
                );
            }
        }
        else
        {
            /*
             * No new telemetry sample was available.
             */
            hw_uart_puts(
                "[TEL] WARNING: No telemetry sample\r\n"
            );
        }


        /*
         * Telemetry runs at 1 Hz.
         *
         * CONTROL continues running at 100 Hz.
         */
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

    /*
     * Save pointer for fault injection.
     */
    g_sensor = &sensor;


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
     * Store controller reference
     * ======================================================== */

    g_controller = &controller;


    hw_uart_puts(
        "[MAIN] Controller initialized\r\n"
    );


    /* ========================================================
     * Create telemetry queue
     * ======================================================== */

    hw_uart_puts(
        "[MAIN] Creating telemetry queue\r\n"
    );

    g_telemetryQueue =
        xQueueCreate(
            1,
            sizeof(TelemetryMessage)
        );


    if (g_telemetryQueue == nullptr)
    {
        hw_uart_puts(
            "[MAIN] ERROR: Telemetry queue creation failed\r\n"
        );

        for (;;)
        {
            __asm volatile ("nop");
        }
    }


    hw_uart_puts(
        "[MAIN] Telemetry queue created successfully\r\n"
    );


    /* ========================================================
     * CONTROL TASK
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
     * TELEMETRY TASK
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
