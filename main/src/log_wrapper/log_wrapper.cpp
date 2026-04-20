#include "src/log_wrapper/log_wrapper.h"
#include <Arduino.h>

LineoutReturn lineout(const char *output) {
    LineoutReturn code = LineoutReturn::Success;

    if (Serial.println(output) == 0) {
        code = LineoutReturn::SerialFailure;
        SerialBT.println("Serial sucks");
    }

    if (SerialBT.println(output) == 0) {
        code = LineoutReturn::SerialBTFailure;
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

    LineoutReturn code = LineoutReturn::Success;

    if (Serial.print(buffer) == 0) {
        code = LineoutReturn::SerialFailure;
    }

    if (SerialBT.print(buffer) == 0) {
        SerialBT.println("couldnt do it :(");
        code = LineoutReturn::SerialBTFailure;
    }

    return code;
}