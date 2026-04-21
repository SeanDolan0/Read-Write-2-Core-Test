
#include <Arduino.h>
#include "src/PWMController/PWMController.h"


/*
 * ESP32 MOSFET PWM Control via SerialBT Monitor
 * MOSFET: FQP30N06L
 */

void PWMSetup() {
  // Configure LEDC PWM
  ledcAttach(pwmPin, freq, resolution);

  SerialBT.println("--- MOSFET PWM Controller ---");
  SerialBT.println("Enter a duty cycle value (0-255):");
}

