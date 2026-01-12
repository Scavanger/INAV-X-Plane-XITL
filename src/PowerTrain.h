#pragma once

#include "platform.h"

#include <vector>

typedef enum
{
    Lipo,
    Lion,
} BatteryChemistryType;

struct BatteryValues
{
    double capacityPercent;
    double voltage;
};

struct MotorValues
{
    double throttlePercent;
    double voltage;
    double current;
    double power;
    int rpm;
    double torque;
    int thrust;
};

namespace PowerTrainConstants
{
    static constexpr double DEFAULT_BATTERY_VOLTAGE = 11.1f; // 3S LiPo
    static constexpr double DEFAULT_BATTERY_CAPACITY_MAH = 2200.0f; // 2200 mAh
    static constexpr int DEFAULT_BATTERY_CELLS = 3;
    static const constexpr double LIPO_INTERNAL_RESISTANCE_PER_CELL = 0.01; // Ohms
    static const constexpr double LION_INTERNAL_RESISTANCE_PER_CELL = 0.015; // Ohms
    static constexpr double LIPO_CUTOFF_VOLTAGE_PER_CELL = 3.2; // Volts
    static constexpr double LION_CUTOFF_VOLTAGE_PER_CELL = 2.5; // Volts

    // T-Motor AT2312 1400 KV @ APC 8x6
    static const std::vector<MotorValues> MOTOR_PERFORMANCE_CURVE = {
        {0.0f,      11.70f,   0.00f,    0.00f,      0,      0.00f,   0   },
        {5.0f,      11.70f,   0.37f,    4.13f,      336,    0.003f, 19   },
        {10.0f,     11.70f,   0.73f,    8.15f,      671,    0.006f, 38   },
        {15.0f,     11.70f,   1.10f,    12.29f,     1006,   0.009f, 57   },
        {20.0f,     11.70f,   1.46f,    16.32f,     1342,   0.012f, 76   },
        {25.0f,     11.70f,   2.19f,    24.48f,     2009,   0.018f, 114  },
        {30.0f,     11.70f,   3.65f,    40.77f,     3354,   0.030f, 190  },
        {35.0f,     11.70f,   4.75f,    53.06f,     4362,   0.039f, 247  },
        {40.0f,     11.70f,   5.85f,    65.19f,     6709,   0.062f, 376  },
        {45.0f,     11.12f,   6.75f,    74.94f,     7075,   0.090f, 422  },
        {50.0f,     11.16f,   7.75f,    86.32f,     7531,   0.076f, 480  },
        {55.0f,     11.13f,   8.82f,    97.91f,     7870,   0.083f, 533  },
        {60.0f,     11.10f,   9.91f,    109.88f,    8179,   0.090f, 592  },
        {65.0f,     11.06f,   11.16f,   123.29f,    8530,   0.098f, 648  },
        {70.0f,     11.03f,   12.52f,   137.88f,    8825,   0.106f, 703  },
        {75.0f,     10.96f,   14.37f,   157.37f,    9237,   0.117f, 777  },
        {80.0f,     10.92f,   16.62f,   181.38f,    9731,   0.130f, 853  },
        {90.0f,     10.81f,   21.79f,   235.278f,   10530,  0.115f, 1009 },
        {100.0f,    10.77f,   23.34f,   251.15f,    10709,  0.161f, 1050 }
    };

    static const std::vector<BatteryValues> LIPO_DISCHARGE_CURVE = {
        {100.0f,    4.20f},
        {95.0f,     4.08f},
        {90.0f,     3.98f},
        {80.0f,     3.88f},
        {70.0f,     3.82f},
        {50.0f,     3.78f},
        {30.0f,     3.70f},
        {20.0f,     3.62f},
        {10.0f,     3.45f},
        {5.0f,      3.20f},
        {0.0f,      3.00f} 
    };

    static const std::vector<BatteryValues> LION_DISCHARGE_CURVE = {
        {100.0f,    4.20f},
        {95.0f,     4.12f},
        {90.0f,     4.07f},
        {80.0f,     4.00f},
        {70.0f,     3.95f},
        {50.0f,     3.85f},
        {30.0f,     3.65f},
        {20.0f,     3.55f},
        {10.0f,     3.35f},
        {5.0f,      3.10f},
        {0.0f,      2.50f} 
    };
}

class PowerTrain
{   

private:
    
    // Battery
    BatteryChemistryType batteryType;
    double batteryCapacityMah;
    double batteryVoltagePerCell = 0.0f;
    double batteryVoltage = 0.0f;
    double cutOffVoltagePerCell = 0.0f;
    int batteryCells = 0;
    double batteryCurrentAmps = 0.0f;
    double batteryCapacityRemainingMah = 0.0f;
    bool batteryDischarged = false;
    std::vector<BatteryValues> dischargeCurve;
    
    // Motor
    double motorMaxOutputWatts = 0.0f;
    double motorReferenceVoltage = 11.1f; // 3S LiPo
    int motorThrottlePercent = 0;
    double motorCurrentAmps = 0.0f;
    int motorRPM = 0;
    double motorPowerWatts = 0.0f;
    double throttleInput = 0.0f;
    double motorThrust = 0.0f;


    void updateBattery(double motorCurrentAmps, double dtSec);
    void updateMotor(double throttlePercent, double voltage, double climbAngleDeg, double dtSec);
    double getBatteryBaseVoltagePerCell(const double capacityPercent) const;
    MotorValues interpolateMotorPerformance(double throttlePercent) const;

public:
    PowerTrain(BatteryChemistryType batteryType = Lipo,
               double batteryCapacityMah = PowerTrainConstants::DEFAULT_BATTERY_CAPACITY_MAH,
               int batteryCells = PowerTrainConstants::DEFAULT_BATTERY_CELLS);

    void update(double throttleInput, double climbAngleDeg, double dtSec);
    
    // Battery
    double getCurrentBatteryVoltage() const { return this->batteryVoltage; };
    double getCurrentBatteryVoltagePerCell() const { return this->batteryVoltagePerCell; };
    double getCurrentBatteryAmps() const { return this->batteryCurrentAmps; };
    double getCurrentBatteryCapacityMah() const { return this->batteryCapacityRemainingMah; };

    bool isBatteryDischarged() const { return this->batteryDischarged; }

    // Motor
    double getMotorThrottleFactor() const { return this->motorPowerWatts / this->motorMaxOutputWatts;};
    int getCurrentMotorRPM() const { return this->motorRPM; };
    double getCurrentMotorPower() const { return this->motorPowerWatts; };
    int getCurrentMotorThrottle() const { return this->motorThrottlePercent; };
    double getCurrentMotorVoltage() const { return this->batteryVoltage; };
    double getCurrentMotorAmps() const { return this->motorCurrentAmps; };
    double getCurrentMotorThrust() const { return this->motorThrust; };
    
};