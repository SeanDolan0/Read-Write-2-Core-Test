# aht30Function

**Files:** `aht30Function.h` · `aht30Function.cpp`

## Overview

High-level Arduino C++ wrapper for the [AHT30](https://asairsensors.com/product/aht30/) temperature and humidity sensor. Delegates to the low-level [`aht30Lib`](../aht30Lib/) driver for I2C communication, and exposes a simple init/read interface for use in the sensor loop.

---

## Dependencies

- **aht30Lib** — Low-level AHT30 driver (see [`aht30Lib/`](../aht30Lib/))

---

## Public API

#### `bool initAht30()`
Calls `aht30_basic_init()` to initialize the sensor over I2C. Prints a success or failure message to `Serial`.

Returns `true` on success, `false` if initialization fails.

#### `std::tuple<float, uint8_t, bool> readAht30()`
Reads temperature and humidity from the sensor via `aht30_basic_read()`.

Returns: `{ temperature_°C, humidity_%, success }`

On failure, returns `{ 0.0f, 0, false }` and prints an error to `Serial`.

---

## Usage Example

```cpp
if (!initAht30()) {
    // handle sensor failure
}

// In your sensor loop:
auto [temp, humidity, ok] = readAht30();
if (ok) {
    writeDataToBuffer("TempIns",  temp);
    writeDataToBuffer("Humidity", (float)humidity);
}
```
