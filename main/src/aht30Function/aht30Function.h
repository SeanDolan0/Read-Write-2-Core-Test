#pragma once

#include <Arduino.h>

extern Adafruit_AHTX0 aht;
extern bool aht_alive;

typedef struct {
    float temperature;
    float humidity;
    bool read_flag;
} AHT_Data_Return;
AHT_Data_Return readAht30();