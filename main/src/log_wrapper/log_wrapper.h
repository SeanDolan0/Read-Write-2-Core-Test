#pragma once

typedef enum {
    Success,
    SerialFailure,
    SerialBTFailure,
    SDFailure,
} LineoutReturn;
LineoutReturn lineout(const char *output);