#include "src/log_wrapper/log_wrapper.h"
#include <Arduino.h>

LineoutReturn lineout(const char *output, bool trailing_newline) {
    LineoutReturn code = LineoutReturn::Success;

    if (trailing_newline) {
        if (Serial.println(output) == 0) {
            code = LineoutReturn::SerialFailure;
        }
    } else {
        if (Serial.print(output) == 0) {
            code = LineoutReturn::SerialFailure;
        }
    }

    if (trailing_newline) {
        if (SerialBT.println(output) == 0) {
            code = LineoutReturn::SerialFailure;
        }
    } else {
        if (SerialBT.print(output) == 0) {
            code = LineoutReturn::SerialFailure;
        }
    }

    // if (!) {}

    return code;
}

LineoutReturn lineoutPrintf(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return lineout(buffer, false);
}