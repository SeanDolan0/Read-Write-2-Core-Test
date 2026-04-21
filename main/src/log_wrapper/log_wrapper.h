#pragma once

#include <BluetoothSerial.h>

extern BluetoothSerial SerialBT;

typedef enum {
    Success,
    SerialFailure,
    SerialBTFailure,
    SDFailure,
} LineoutReturn;

LineoutReturn lineout(const char *output, bool trailing_newline = true);
LineoutReturn lineoutPrintf(const char *format, ...);