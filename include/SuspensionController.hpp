#ifndef SUSPENSION_CONTROLLER_HPP
#define SUSPENSION_CONTROLLER_HPP

#include <memory>

#include "ISensorReader.hpp"
#include "ICoilDriver.hpp"
#include "ITelemetryLogger.hpp"
#include "IDampingStrategy.hpp"
#include "SignalFilter.hpp"
#include "RingBuffer.hpp"


class SuspensionController
{
public:

    SuspensionController(
        ISensorReader& sensor,
        ICoilDriver& coil,
        ITelemetryLogger& logger
    ) noexcept;


    void setStrategy(
        std::unique_ptr<IDampingStrategy> strategy
    ) noexcept;


    void runCycle(
        float ambientTempC
    ) noexcept;


    /* ========================================================
     * SAFE MODE
     * ======================================================== */

    void enterSafeMode(
        SensorError error
    ) noexcept;


    void enterSafeMode(
        CoilError error
    ) noexcept;


    /*
     * Explicit recovery from a latched coil fault.
     *
     * This is intentionally NOT automatic.
     *
     * The application/test must explicitly call this
     * after the coil fault has been cleared.
     */
    void clearCoilFault() noexcept;


    /* ========================================================
     * TELEMETRY GETTERS
     * ======================================================== */

    [[nodiscard]]
    float lastAccelerationG() const noexcept
    {
        return lastAccelerationG_;
    }


    [[nodiscard]]
    float lastForceN() const noexcept
    {
        return lastForceN_;
    }


    [[nodiscard]]
    float lastRequestedCurrentA() const noexcept
    {
        return lastRequestedCurrentA_;
    }


    [[nodiscard]]
    float lastTemperatureC() const noexcept
    {
        return lastTemperatureC_;
    }


    [[nodiscard]]
    SensorError lastSensorError() const noexcept
    {
        return lastSensorError_;
    }


    [[nodiscard]]
    CoilError lastCoilError() const noexcept
    {
        return lastCoilError_;
    }


    [[nodiscard]]
    bool isSafeMode() const noexcept
    {
        return safeMode_;
    }


    [[nodiscard]]
    const RingBuffer<float, 32>&
    getRecentForceHistory() const noexcept
    {
        return forceHistory_;
    }


private:

    ISensorReader& sensor_;

    ICoilDriver& coil_;

    ITelemetryLogger& logger_;


    std::unique_ptr<IDampingStrategy> strategy_;


    /* Fixed-size filter. */
    SignalFilter<5> filter_;


    /* Fixed-capacity force history. */
    RingBuffer<float, 32> forceHistory_{};


    float lastAccelerationG_{0.0f};

    float lastForceN_{0.0f};

    float lastRequestedCurrentA_{0.0f};

    float lastTemperatureC_{25.0f};


    SensorError lastSensorError_{
        SensorError::None
    };


    CoilError lastCoilError_{
        CoilError::None
    };


    bool safeMode_{false};

    /*
     * A coil fault is latched until an explicit
     * clearCoilFault() call is made.
     */
    bool coilFaultLatched_{false};
};


#endif