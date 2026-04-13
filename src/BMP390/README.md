# BMP390

**Files:** `BMP390Function.h` · `BMP390Function.cpp`

## Overview

Thin wrapper around the [BMP390](https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/bmp390/) barometric pressure and temperature sensor. The sensor is initialized over I2C and readings are returned as a `bmp3_data` struct for use in the SD logging pipeline.

---

## Hardware Configuration

| Setting      | Value    | Description                     |
|--------------|----------|---------------------------------|
| SDA pin      | GPIO 21  | I2C data line                   |
| SCL pin      | GPIO 22  | I2C clock line                  |

---

## Dependencies

- **BMP390** — Arduino BMP390 library (`BMP390.h`)

---

## Public API

#### `void initBMP390()`
Initializes the BMP390 sensor. Calls `bmp.get_bmp_values()` and halts (`while(1)`) if the sensor is not found or fails to respond.

#### `bmp3_data Temp_Presure_Write_To_SD()`
Reads the current temperature and pressure from the sensor and returns a `bmp3_data` struct. The caller is responsible for forwarding the values to [`SdFunction`](../SdFunction/) via `writeDataToBuffer()`.

---

## Usage Example

```cpp
initBMP390();

// In your sensor loop:
bmp3_data data = Temp_Presure_Write_To_SD();
writeDataToBuffer("BaroTempIns", data.temperature);
writeDataToBuffer("PressIns",    data.pressure);
```
