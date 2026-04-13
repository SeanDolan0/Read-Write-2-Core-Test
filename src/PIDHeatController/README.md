# PIDHeatController

**Files:** `PIDHeatController.h` · `PIDHeatController.cpp`

## Overview

A discrete-time PID (Proportional-Integral-Derivative) controller used to regulate a heater output. Given a target temperature, the current temperature, and the elapsed time since the last update, it computes a clamped output value suitable for driving a PWM heater channel (see [`PWMController/`](../PWMController/)).

---

## Configuration

The following constants are defined in `PIDHeatController.h` and control the controller's behavior:

| Constant             | Default | Description                                      |
|----------------------|---------|--------------------------------------------------|
| `kp`                 | `20.0`  | Proportional gain                                |
| `ki`                 | `1.0`   | Integral gain                                    |
| `kd`                 | `5.0`   | Derivative gain                                  |
| `targetTemperature`  | `30.0`  | Desired temperature in °C                        |
| `maxVal`             | `255.0` | Maximum output value (matches 8-bit PWM range)   |

---

## Dependencies

- **Sensors.h** — `extern` declarations for the PID constants (`kp`, `ki`, `kd`, `targetTemperature`, `maxVal`)

---

## Public API

#### `float CalculatePID(float target, float current, float dt)`
Computes the PID output for one time step.

| Parameter | Description                                   |
|-----------|-----------------------------------------------|
| `target`  | Desired temperature (°C)                      |
| `current` | Current measured temperature (°C)             |
| `dt`      | Time elapsed since last call (seconds)        |

Returns a float clamped to `[0, maxVal]`.

Internally maintains `integral` and `last_error` state across calls. Reset these if the controller is paused or the target changes significantly.

---

## Usage Example

```cpp
float dt = (millis() - lastPIDTime) / 1000.0f;
lastPIDTime = millis();

float output = CalculatePID(targetTemperature, currentTemp, dt);
ledcWrite(ledChannel, (uint8_t)output);
```
