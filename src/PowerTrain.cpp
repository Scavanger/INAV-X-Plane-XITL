#include "PowerTrain.h"
#include <algorithm>
#include <cmath>

PowerTrain::PowerTrain(BatteryChemistryType batteryType, double batteryCapacityMah, int batteryCells) : batteryType(batteryType),
                                                                                                        batteryCapacityMah(batteryCapacityMah),
                                                                                                        batteryCells(batteryCells)
{
    this->batteryCapacityRemainingMah = batteryCapacityMah;
    if (batteryType == Lipo)
    {
        this->dischargeCurve = PowerTrainConstants::LIPO_DISCHARGE_CURVE;
        this->cutOffVoltagePerCell = PowerTrainConstants::LIPO_CUTOFF_VOLTAGE_PER_CELL;
    }
    else if (batteryType == Lion)
    {
        this->dischargeCurve = PowerTrainConstants::LION_DISCHARGE_CURVE;
        this->cutOffVoltagePerCell = PowerTrainConstants::LION_CUTOFF_VOLTAGE_PER_CELL;
    }

    this->motorMaxOutputWatts = PowerTrainConstants::MOTOR_PERFORMANCE_CURVE.back().power;
}

void PowerTrain::update(double throttleInput, double climbAngleDeg, double dtSec)
{
    const double throttlePercent = std::clamp(throttleInput * 100.0, 0.0, 100.0);
    this->updateBattery(this->motorCurrentAmps, dtSec);
    this->updateMotor(throttlePercent, this->batteryVoltage, climbAngleDeg, dtSec);
}
    
void PowerTrain::updateBattery(double motorCurrentAmps, double dtSec)
{
    this->batteryCurrentAmps = motorCurrentAmps;
    this->batteryCapacityRemainingMah -= motorCurrentAmps * (dtSec / 3600.0f) * 1000.0f; // convert to mAh

    if (this->batteryCapacityRemainingMah <= 0.0f)
    {
        this->batteryCapacityRemainingMah = 0.0f;
        this->batteryDischarged = true;
        this->batteryVoltagePerCell = 0.0f;
        this->batteryVoltage = 0.0f;
        return;
    }

    const double capacityPercent = (this->batteryCapacityRemainingMah / this->batteryCapacityMah) * 100.0f;
    const double baseVoltagePerCell = this->getBatteryBaseVoltagePerCell(capacityPercent);
    const double internalResistancePerCell = (this->batteryType == Lipo) ? PowerTrainConstants::LIPO_INTERNAL_RESISTANCE_PER_CELL
                                                                       : PowerTrainConstants::LION_INTERNAL_RESISTANCE_PER_CELL;
                                              
    const double voltageDropPerCell = motorCurrentAmps * internalResistancePerCell;
    this->batteryVoltagePerCell = baseVoltagePerCell - voltageDropPerCell;
    this->batteryVoltage = this->batteryVoltagePerCell * this->batteryCells;

    if (this->batteryVoltage <= this->cutOffVoltagePerCell * this->batteryCells)
    {
        this->batteryDischarged = true;
    }
}

void PowerTrain::updateMotor(double throttlePercent, double voltage, double climbAngleDeg, double dtSec)
{
    if (this->batteryDischarged)
    {
        this->motorRPM = 0;
        this->motorCurrentAmps = 0.0f;
        this->motorPowerWatts = 0.0f;
        this->motorThrust = 0.0f;
        return;
    }
    
    const double climbAngleRad = climbAngleDeg * (std::numbers::pi / 180.0);

    // Calculate load multiplier based on climb angle
    // Positive angle = climb, negative = descent
    // Load factor = 1 + k * sin(angle)
    // At 0°: factor = 1 (no extra load)
    // At +90°: factor = 1.5 (50% more load for vertical climb)
    // At -90°: factor = 0.5 (50% less load for vertical descent)
    const double loadFactor = 1 + 0.5 * std::sin(climbAngleRad); 
    const MotorValues motorBaseValues = this->interpolateMotorPerformance(throttlePercent);
    const double voltageRatio = voltage / this->motorReferenceVoltage;
    const double effectiveVoltageRatio = voltageRatio / loadFactor;
    this->motorRPM = static_cast<int>(motorBaseValues.rpm * effectiveVoltageRatio);
    this->motorCurrentAmps = motorBaseValues.current * voltageRatio * loadFactor;
    this->motorPowerWatts = voltage * this->motorCurrentAmps;
    this->motorThrust = motorBaseValues.thrust * effectiveVoltageRatio;
}

double PowerTrain::getBatteryBaseVoltagePerCell(const double capacityPercent) const
{
    if (capacityPercent >= 100.0f)
    {
        return this->dischargeCurve.front().voltage;
    } 
    else if (capacityPercent <= 0.0f)
    {
        return this->dischargeCurve.back().voltage;
    }   

    for (size_t i = 1; i < this->dischargeCurve.size(); ++i)
    {
        if (capacityPercent >= this->dischargeCurve[i].capacityPercent)
        {
            const BatteryValues& upper = this->dischargeCurve[i - 1];
            const BatteryValues& lower = this->dischargeCurve[i];

            // Linear interpolation
            double slope = (upper.voltage - lower.voltage) / (upper.capacityPercent - lower.capacityPercent);
            return lower.voltage + slope * (capacityPercent - lower.capacityPercent);
        }
    }
    
    return 0.0;
}

MotorValues PowerTrain::interpolateMotorPerformance(double throttlePercent) const
{
    for (size_t i = 1; i < PowerTrainConstants::MOTOR_PERFORMANCE_CURVE.size(); ++i)
    {
        if (throttlePercent <= PowerTrainConstants::MOTOR_PERFORMANCE_CURVE[i].throttlePercent)
        {
            const MotorValues& upper = PowerTrainConstants::MOTOR_PERFORMANCE_CURVE[i - 1];
            const MotorValues& lower = PowerTrainConstants::MOTOR_PERFORMANCE_CURVE[i];

            // Linear interpolation
            double slope = (throttlePercent - lower.throttlePercent) / (upper.throttlePercent - lower.throttlePercent);

            MotorValues result;
            result.throttlePercent = throttlePercent;
            result.voltage = lower.voltage + slope * (upper.voltage - lower.voltage);
            result.current = lower.current + slope * (upper.current - lower.current);
            result.power = lower.power + slope * (upper.power - lower.power);
            result.rpm = static_cast<int>(lower.rpm + slope * (upper.rpm - lower.rpm));
            result.torque = lower.torque + slope * (upper.torque - lower.torque);
            result.thrust = static_cast<int>(lower.thrust + slope * (upper.thrust - lower.thrust));

            return result;
        }
    }
    
    return MotorValues();
}
