# PWMController

**Files:** `PWMController.h` · `PWMController.cpp`

## Overview

Configures the ESP32's LEDC (LED Control) peripheral to generate a PWM signal for driving a [FQP30N06L](https://www.onsemi.com/products/discrete-power-modules/mosfets/fqp30n06l) N-channel MOSFET. This is used to control a resistive heater or other DC load via the duty cycle. Intended to be used alongside [`PIDHeatController`](../PIDHeatController/) which calculates the duty cycle value.

---

## Hardware Configuration

| Setting        | Value      | Description                                         |
|----------------|------------|-----------------------------------------------------|
| `pwmPin`       | GPIO 13    | Connected to the MOSFET gate                        |
| `freq`         | 5000 Hz    | PWM carrier frequency                               |
| `ledChannel`   | 0          | ESP32 LEDC channel (0–15)                           |
| `resolution`   | 8 bits     | Duty cycle range: 0–255                             |

---

## Dependencies

- **BluetoothFunction** — `SerialBT` used for user prompts (see [`BluetoothFunction/`](../BluetoothFunction/))

---

## Public API

#### `void PWMSetup()`
Attaches `pwmPin` to `ledChannel` using `ledcSetup()` and `ledcAttachPin()`, then prints a prompt over `SerialBT` instructing the user to enter a duty cycle value (0–255).

After calling `PWMSetup()`, use `ledcWrite(ledChannel, value)` to update the duty cycle.

---

## Usage Example

```cpp
PWMSetup();

// In your control loop, driven by PID output:
float dt = (millis() - lastTime) / 1000.0f;
lastTime = millis();

float pidOutput = CalculatePID(targetTemperature, currentTemp, dt);
ledcWrite(ledChannel, (uint8_t)pidOutput);
```
