#ifndef SUSPENSION_CONTROLLER_HPP
#define SUSPENSION_CONTROLLER_HPP

#include <memory>

#include "ISensorReader.hpp"
#include "ICoilDriver.hpp"
#include "ITelemetryLogger.hpp"
#include "IDampingStrategy.hpp"
#include "SignalFilter.hpp"
#include "RingBuffer.hpp"


/* ============================================================
 * CONTROLLER STATE
 * ============================================================ */

enum class ControllerState
{
    Normal,
    SensorSafeMode,
    CoilSafeMode
};


/* ============================================================
 * SUSPENSION CONTROLLER
 * ============================================================ */

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


    /* ========================================================
     * EXPLICIT COIL FAULT RECOVERY
     * ======================================================== */

    void clearCoilFault() noexcept;


    /* ========================================================
     * STATE
     * ======================================================== */

    [[nodiscard]]
    ControllerState state() const noexcept
    {
        return state_;
    }


    [[nodiscard]]
    bool isSafeMode() const noexcept
    {
        return state_ != ControllerState::Normal;
    }


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
    const RingBuffer<float, 32>&
    getRecentForceHistory() const noexcept
    {
        return forceHistory_;
    }


private:

    /* ========================================================
     * HARDWARE / INTERFACES
     * ======================================================== */

    ISensorReader& sensor_;

    ICoilDriver& coil_;

    ITelemetryLogger& logger_;


    /* ========================================================
     * CONTROL STRATEGY
     * ======================================================== */

    std::unique_ptr<IDampingStrategy> strategy_;


    /* ========================================================
     * SIGNAL PROCESSING
     * ======================================================== */

    SignalFilter<5> filter_;


    /* ========================================================
     * FORCE HISTORY
     * ======================================================== */

    RingBuffer<float, 32> forceHistory_{};


    /* ========================================================
     * CONTROLLER STATE
     * ======================================================== */

    ControllerState state_{
        ControllerState::Normal
    };


    /* ========================================================
     * TELEMETRY STATE
     * ======================================================== */

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
};


#endif