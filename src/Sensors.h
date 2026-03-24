#pragma once

#include <Arduino.h>
#include "SdFunction/SdFunction.h"
#include "RockblockFunction/RockblockFunction.h"
#include <freertos/FreeRTOS.h>

constexpr float InvalidTemperature = -512.0f;
constexpr float InvalidHumidity = -1.0f;
constexpr float InvalidPressure = -1.0f;

constexpr char* const SENSOR_NAMES[] = {"BMP390_temperature", "BMP390_pressure", "ATH30_temperature", "ATH30_humidity"};
constexpr float const INVALID_RESPONSES[] = {InvalidTemperature, InvalidPressure, InvalidTemperature, InvalidHumidity};
constexpr int NUM_SENSORS = sizeof(SENSOR_NAMES) / sizeof(SENSOR_NAMES[0]);
