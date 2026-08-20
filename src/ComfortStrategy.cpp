#include "ComfortStrategy.hpp"


float ComfortStrategy::calculateForceN(
    float accelG) const noexcept
{
    return accelG * 50.0f;
}


DampingCommand ComfortStrategy::calculate(
    const SensorData& sensorData) const
{
    const float force =
        calculateForceN(
            sensorData.accelerationG
        );


    return DampingCommand{
        force,
        force * 0.1f
    };
}