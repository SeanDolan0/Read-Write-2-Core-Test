#pragma once

#include <Arduino.h>

// ─────────────────────────────────────────────
//  Sensor registry
//  Add new sensors here only — nothing else needs changing
// ─────────────────────────────────────────────
typedef enum : uint16_t {
    SENSOR_TEMP      = (1 << 0),
    SENSOR_PRESSURE  = (1 << 1),
    SENSOR_HUMIDITY  = (1 << 2),
    // SENSOR_ACCEL_X = (1 << 3),  // uncomment to add more
    // SENSOR_ACCEL_Y = (1 << 4),
    // SENSOR_BATTERY = (1 << 5),
} SensorID;

// Fixed-order slot index for each sensor (must match SensorID bit order)
typedef enum {
    SLOT_TEMP     = 0,
    SLOT_PRESSURE = 1,
    SLOT_HUMIDITY = 2,
    // SLOT_ACCEL_X = 3,
    // SLOT_ACCEL_Y = 4,
    // SLOT_BATTERY = 5,
    MAX_SENSORS   = 3,   // <-- update this when adding sensors
} SensorSlot;

// Scaling factors: raw int16 stored = actual_float * SCALE
// Lets us avoid floats in the binary message
static const int16_t SENSOR_SCALE[MAX_SENSORS] = {
    10,   // temp:     stored as tenths of a degree  (e.g. 253 = 25.3°C)
    10,   // pressure: stored as tenths of a hPa     (e.g. 10013 = 1001.3 hPa)
    10,   // humidity: stored as tenths of a percent (e.g. 456 = 45.6%)
};

// Human-readable names (used for CSV header and ASCII logging)
static const char* const SENSOR_NAMES[MAX_SENSORS] = {
    "temp",
    "pressure",
    "humidity",
};

// ─────────────────────────────────────────────
//  The canonical in-memory record
// ─────────────────────────────────────────────
struct SensorRecord {
    uint32_t timestamp_ms;
    uint16_t sensor_mask;              // which slots are valid
    int16_t  values[MAX_SENSORS];      // scaled integer values, fixed-order slots
};

// ─────────────────────────────────────────────
//  Shared ring buffer (defined in SdFunction.cpp)
// ─────────────────────────────────────────────
#define MAX_RECORDS 256   // must be power of 2 for fast wrap

extern SensorRecord recordBuffer[MAX_RECORDS];
extern volatile uint16_t recordHead;   // next slot to write into
extern volatile uint16_t sdTail;       // SD has consumed up to here
extern volatile uint16_t rbTail;       // RockBlock has consumed up to here
