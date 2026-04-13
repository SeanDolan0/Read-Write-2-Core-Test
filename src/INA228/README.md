# INA228

**Files:** `INA.h` · `INA.cpp`

## Overview

Wrapper around the [INA228](https://www.ti.com/product/INA228) 20-bit power/energy monitor using the [Adafruit INA228 library](https://github.com/adafruit/Adafruit_INA228). Measures bus voltage (V) and load current (mA) through a shunt resistor and returns them as a tuple. Diagnostic output is sent over Bluetooth Serial (`SerialBT`).

---

## Hardware Configuration

| Setting                    | Value         | Description                                 |
|----------------------------|---------------|---------------------------------------------|
| Shunt resistance           | 0.015 Ω       | Configured via `ina228.setShunt()`           |
| Maximum expected current   | 10 A          | Configured via `ina228.setShunt()`           |
| Averaging count            | 16 samples    | `INA228_COUNT_16`                            |
| Voltage conversion time    | 150 µs        | `INA228_TIME_150_us`                         |
| Current conversion time    | 280 µs        | `INA228_TIME_280_us`                         |

---

## Dependencies

- **Adafruit_INA228** — Adafruit INA228 Arduino library
- **BluetoothFunction** — `SerialBT` used for diagnostic output (see [`BluetoothFunction/`](../BluetoothFunction/))

---

## Public API

#### `void initializeINA228()`
Initializes the INA228 over I2C. Sets shunt parameters, averaging count, and conversion times. Prints chip details and configuration to `SerialBT`. Halts (`while(1)`) if the chip is not detected.

#### `std::tuple<float, float> ReadINA228()`
Reads the current bus voltage and load current. Prints both values to `SerialBT`.

Returns: `{ current_mA, busVoltage_V }`

---

## Usage Example

```cpp
initializeINA228();

// In your sensor loop:
auto [current, voltage] = ReadINA228();
writeDataToBuffer("hCurrent", current);
writeDataToBuffer("hVoltage", voltage);
```
