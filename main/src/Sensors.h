#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <limits>
#include "SdFunction/SdFunction.h"
#include <freertos/FreeRTOS.h>

/* ------------------------------ Sensor Status ----------------------------- */

extern bool bmp_outside_alive;
extern bool bmp_inside_alive;
extern bool fxos_fxas_alive;
extern bool mcp_alive;
extern bool aht_alive;
extern bool ina_low_alive;
extern bool ina_high_alive;

/* ------------------------------ PID Constants ----------------------------- */

extern float kp;
extern float ki;
extern float kd;
extern float tragetTemperature;
extern float maxVal;

/* ---------------------------- Invalid Constants --------------------------- */

constexpr float InvalidTemperature = -512.0f;
constexpr float InvalidHumidity = -1.0f;
constexpr float InvalidPressure = -1.0f;
constexpr float InvalidPosition = std::numeric_limits<float>::infinity();


constexpr float const INVALID_RESPONSES[] = { 
    InvalidTemperature,
    InvalidTemperature,
    InvalidTemperature,
    InvalidTemperature,
    InvalidTemperature,

    InvalidPressure,
    InvalidPressure,
 
    InvalidHumidity,

    InvalidPosition, 
    InvalidPosition, 

    InvalidPosition, 
    InvalidPosition,

    InvalidPosition, InvalidPosition, InvalidPosition,
    InvalidPosition, InvalidPosition,
    InvalidPosition, InvalidPosition, InvalidPosition
}; 

/* ------------------------------ Sensor Names ------------------------------ */

// Sensor count needs to be last
typedef enum {
    AhtTemperature,
    AhtHumidity,

    GyroRoll,
    GyroPitch,
    GyroYaw,
    GyroLinAccX,

    InsBmpTemp,
    InsBmpPress,
    OutBmpTemp,
    OutBmpPress,

    McpTempF,
    McpTempC,

    LowInaBusVolt,
    LowInaCurrent,
    HighInaBusVolt,
    HighInaCurrent,

    SENSOR_COUNT,
} SensorDataType;

inline const char *get_sensor_name(SensorDataType type) {
    static const char *names[] = {
        "AhtTemperature",
        "AhtHumidity",

        "GyroRoll",
        "GyroPitch",
        "GyroYaw",
        "GyroLinAccX",
        "GyroLinAccY",
        "GyroLinAccZ",

        "InsBmpTemp",
        "InsBmpPress",
        "OutBmpTemp",
        "OutBmpPress",

        "McpTempF",
        "McpTempC",

        "LowInaBusVolt",
        "LowInaCurrent",
        "HighInaBusVolt",
        "HighInaCurrent",
    };
    return names[type];
}

#endif