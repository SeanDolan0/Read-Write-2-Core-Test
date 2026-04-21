#pragma once

#ifndef LOG_WRAPPER_H
#define LOG_WRAPPER_H

#include <BluetoothSerial.h>

extern BluetoothSerial SerialBT;

enum class LineoutReturn
{
    Success,
    SerialFailure,
    SerialBTFailure,
    SDFailure,
};

LineoutReturn lineout(const char *output, bool trailing_newline = true);
LineoutReturn lineoutPrintf(const char *format, ...);

#endif