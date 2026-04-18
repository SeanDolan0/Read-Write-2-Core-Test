# SdFunction

**Files:** `SdFunction.h` · `SdFunction.cpp`

## Overview

Buffers sensor readings in a heap-allocated 128 KB RAM buffer for efficient CSV logging to SD card, and maintains a parallel [RockBLOCK](../RockblockFunction/) table buffer for periodic Iridium SBD satellite transmission. Both channels are flushed on independent time-based intervals and share a FreeRTOS mutex for SD access.

---

## Runtime Configuration

| Constant / Variable           | Value              | Description                                         |
|-------------------------------|--------------------|-----------------------------------------------------|
| `SD_CS_PIN`                   | `5`                | SPI chip select pin for the SD card                 |
| `CSV_FILE_PATH`               | `"/sensor_log.csv"`| File path on the SD card                            |
| `WRITE_INTERVAL_MS`           | `60000 ms`         | How often `LogWriteBuffer()` should be called       |
| `ROCKBLOCK_SEND_INTERVAL_MS`  | `60000 ms`         | Minimum interval between satellite send attempts    |
| `MUTEX_TIMEOUT_MS`            | `5000 ms`          | FreeRTOS semaphore acquisition timeout              |
| `CSV_LOG_BUFFER_SIZE`         | `131072 bytes`     | 128 KB heap-allocated RAM buffer for pending CSV rows |

**CSV columns (in order):** `timestamp_ms`, then one column per `SensorDataType` enum value from `Sensors.h` (e.g. `TempIns`, `TempOut`, `PressIns`, `PressOut`, `Humidity`, …). Each row only fills in the column matching the sensor that was written; all other columns are left blank.

---

## Dependencies

- **SD / SPI** — Arduino SD card library
- **FreeRTOS** — `logMutex` (`SemaphoreHandle_t`) must be created by the application before any SD operations
- **RockblockFunction** — `Table`, `TableEntry`, `add_entry()`, `send_table()`, etc. (see [`RockblockFunction/`](../RockblockFunction/))
- **Sensors.h** — `SensorDataType` enum and `get_sensor_name()` used to build CSV column headers

> `logMutex` is declared `extern` in `SdFunction.h` — the application must call `logMutex = xSemaphoreCreateMutex()` during setup.

---

## Public API

### Initialization

#### `bool initSDCard()`
Mounts the SD card on `SD_CS_PIN` and prints the card size over `Serial`. Builds the column name array from the `SensorDataType` enum. If `sensor_log.csv` does not yet exist, creates it and writes the CSV header row. Returns `false` on any failure.

#### `bool initRockblockBuffer()`
Heap-allocates `csvLogBuffer` (`CSV_LOG_BUFFER_SIZE` bytes) and allocates the internal `rockblockTable` via `new_table()` if it has not already been created. Returns `false` if allocation fails.

---

### Data Ingestion

#### `void writeDataToBuffer(const char* name, float value)`
The main data ingestion point. Accepts a sensor name string (must match a `SensorDataType` name from `Sensors.h`) and its reading. On each call it:

1. **RockBLOCK path** — If `rockblockTable` exists and adding one `TableEntry` would not exceed 340 bytes, appends a `TableEntry` with the matched `SensorDataType` index and current `millis()` timestamp.
2. **CSV path** — Formats a sparse row as `timestamp_ms,<blank columns except the matching sensor>` and appends it to `csvLogBuffer`. If the buffer would overflow, automatically calls `LogWriteBuffer()` to flush to SD before appending.

Both paths acquire `logMutex` independently. Unrecognized `name` values are logged to `Serial` and otherwise silently ignored.

---

### Flushing

#### `bool LogWriteBuffer()`
Acquires `logMutex`, appends all pending bytes in `csvLogBuffer` to `sensor_log.csv` using `FILE_APPEND`, then clears the buffer. Returns `false` if the mutex cannot be acquired, the file cannot be opened, or a partial write is detected.

#### `bool sendRockblockBuffer()`
If `ROCKBLOCK_SEND_INTERVAL_MS` has elapsed since the last send and the table has at least one entry, calls `send_table()`, frees the old table, and allocates a fresh one. Returns `true` if the interval has not yet elapsed; returns `false` if the new table allocation fails.

---

### Helpers

#### `void randomSensorData()`
Generates random temperature and pressure readings at staggered intervals and passes them to `writeDataToBuffer()`. Used for testing the buffer and SD logging pipeline without real sensors attached.

---

## Expected Usage Flow

```cpp
// 1. Application setup
logMutex = xSemaphoreCreateMutex();
initSDCard();
initRockblockBuffer();

// 2. As sensor data arrives (from any task/ISR-safe context)
writeDataToBuffer("TempIns", readTemp());
writeDataToBuffer("PressIns", readPressure());

// 3. Periodic flush tasks (e.g. in a FreeRTOS task or main loop)
if (millis() - lastWriteTime >= WRITE_INTERVAL_MS) {
    LogWriteBuffer();
    lastWriteTime = millis();
}
sendRockblockBuffer(); // no-ops silently until interval elapses
```

---

## Internal State

| Variable               | Type           | Description                                      |
|------------------------|----------------|--------------------------------------------------|
| `csvLogBuffer`         | `char*`        | Heap-allocated RAM buffer for pending CSV rows   |
| `logBufferlen`         | `size_t`       | Number of valid bytes currently in `csvLogBuffer`|
| `sdReady`              | `bool`         | SD mount status (set externally or by app)       |
| `lastWriteTime`        | `uint32_t`     | `millis()` timestamp of last SD write            |
| `rockblockTable`       | `Table*`       | Active RockBLOCK entry buffer (module-internal)  |
| `lastRockblockSendTime`| `uint32_t`     | `millis()` timestamp of last satellite send      |
