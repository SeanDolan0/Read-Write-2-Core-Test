# aht30Lib

**Files:** `driver_aht30.h` · `driver_aht30.c` · `driver_aht30_basic.h` · `driver_aht30_basic.c` · `driver_aht30_interface.h` · `driver_aht30_interface.cpp`

## Overview

Low-level C driver for the [AHT30](https://asairsensors.com/product/aht30/) temperature and humidity sensor, based on the [LibDriver](https://github.com/libdriver/aht30) open-source driver (MIT license, © LibDriver). Communicates via I2C. The driver is split into three layers:

| Layer         | Files                                    | Responsibility                                           |
|---------------|------------------------------------------|----------------------------------------------------------|
| Core driver   | `driver_aht30.h` / `.c`                 | AHT30 protocol logic; status, read, calibration         |
| Basic API     | `driver_aht30_basic.h` / `.c`           | Simplified init/deinit/read wrappers                    |
| Interface     | `driver_aht30_interface.h` / `.cpp`     | Platform bindings (I2C, delay, debug print) for ESP32   |

The [`aht30Function`](../aht30Function/) module is the recommended entry point for use in this project.

---

## Basic API (Recommended)

These three functions are the only ones needed for typical use:

#### `uint8_t aht30_basic_init(void)`
Initializes the sensor. Wires up the interface callbacks (I2C, delay, debug print) and calls the core `aht30_init()`. Returns `0` on success, `1` on failure.

#### `uint8_t aht30_basic_deinit(void)`
Deinitializes the sensor and releases the I2C bus. Returns `0` on success.

#### `uint8_t aht30_basic_read(float* temperature, uint8_t* humidity)`
Reads temperature (°C) and relative humidity (%) from the sensor. Returns `0` on success, `1` on failure.

---

## Core Driver API

#### `uint8_t aht30_init(aht30_handle_t* handle)`
Initializes the chip. The `handle` must have all interface function pointers set (via the `DRIVER_AHT30_LINK_*` macros) before calling. Returns `0` on success.

#### `uint8_t aht30_deinit(aht30_handle_t* handle)`
Releases resources and deinitializes I2C.

#### `uint8_t aht30_read_temperature_humidity(aht30_handle_t* handle, uint32_t* temperature_raw, float* temperature_s, uint32_t* humidity_raw, uint8_t* humidity_s)`
Reads both temperature and humidity, returning raw and converted values.

#### `uint8_t aht30_read_temperature(aht30_handle_t* handle, uint32_t* temperature_raw, float* temperature_s)`
Reads temperature only.

#### `uint8_t aht30_read_humidity(aht30_handle_t* handle, uint32_t* humidity_raw, uint8_t* humidity_s)`
Reads humidity only.

#### `uint8_t aht30_get_status(aht30_handle_t* handle, uint8_t* status)`
Returns the raw AHT30 status byte (busy, mode, calibration flags — see `aht30_status_t` enum).

#### `uint8_t aht30_info(aht30_info_t* info)`
Fills an `aht30_info_t` struct with chip metadata (name, manufacturer, voltage range, driver version).

---

## Interface Layer

The interface layer (`driver_aht30_interface.cpp`) provides the platform-specific bindings that the core driver calls through function pointers:

| Function                        | Description                          |
|---------------------------------|--------------------------------------|
| `aht30_interface_iic_init()`    | Initialize the I2C bus               |
| `aht30_interface_iic_deinit()`  | Release the I2C bus                  |
| `aht30_interface_iic_read_cmd()`| Read bytes from the sensor           |
| `aht30_interface_iic_write_cmd()`| Write bytes to the sensor           |
| `aht30_interface_delay_ms()`    | Millisecond delay                    |
| `aht30_interface_debug_print()` | Printf-style debug output            |

---

## Handle Initialization (Advanced Use)

If using the core driver directly instead of the Basic API:

```c
aht30_handle_t handle;

DRIVER_AHT30_LINK_INIT(&handle, aht30_handle_t);
DRIVER_AHT30_LINK_IIC_INIT(&handle, aht30_interface_iic_init);
DRIVER_AHT30_LINK_IIC_DEINIT(&handle, aht30_interface_iic_deinit);
DRIVER_AHT30_LINK_IIC_READ_CMD(&handle, aht30_interface_iic_read_cmd);
DRIVER_AHT30_LINK_IIC_WRITE_CMD(&handle, aht30_interface_iic_write_cmd);
DRIVER_AHT30_LINK_DELAY_MS(&handle, aht30_interface_delay_ms);
DRIVER_AHT30_LINK_DEBUG_PRINT(&handle, aht30_interface_debug_print);

aht30_init(&handle);
```
