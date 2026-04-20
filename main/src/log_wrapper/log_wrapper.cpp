#include "src/log_wrapper/log_wrapper.h"
#include <Arduino.h>

LineoutReturn lineout(const char *output) {
    if (Serial.println(output) == 0) {
        return LineoutReturn::SerialFailure;
    }

    if (SerialBT.println(output) == 0) {
        return LineoutReturn::SerialBTFailure;
    }

    // if (!) {}

    return LineoutReturn::Success;
}