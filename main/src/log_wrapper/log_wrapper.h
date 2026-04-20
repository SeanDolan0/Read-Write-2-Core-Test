#pragma once

#include <BluetoothSerial.h>

extern BluetoothSerial SerialBT;

typedef enum {
    Success,
    SerialFailure,
    SerialBTFailure,
    SDFailure,
} LineoutReturn;

LineoutReturn lineout(const char *output);
LineoutReturn lineoutPrintf(const char *format, ...);