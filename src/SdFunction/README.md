# SdFunction

**Files:** `SdFunction.h` · `SdFunction.cpp`

## Overview

Buffers sensor readings in a 64 KB RAM buffer for efficient CSV logging to SD card, and maintains a parallel [RockBLOCK](../RockblockFunction/) table buffer for periodic Iridium SBD satellite transmission. Both channels are flushed on independent time-based intervals and share a FreeRTOS mutex for SD access.

---

## Runtime Configuration

| Constant / Variable           | Value              | Description                                         |
|-------------------------------|--------------------|-----------------------------------------------------|
| `SD_CS_PIN`                   | `5`                | SPI chip select pin for the SD card                 |
| `CSV_FILE_PATH`               | `"/sensor_log.csv"`| File path on the SD card                            |
| `WRITE_INTERVAL_MS`           | `60000 ms`         | How often `LogWriteBuffer()` should be called       |
| `ROCKBLOCK_SEND_INTERVAL_MS`  | `60000 ms`         | Minimum interval between satellite send attempts    |
| `MUTEX_TIMEOUT_MS`            | `5000 ms`          | FreeRTOS semaphore acquisition timeout              |
| `LogBuffer` size              | `65536 bytes`      | 64 KB RAM buffer for pending CSV rows               |

**CSV columns (in order):** `timestamp_ms`, `temp`, `pressure`

---

## Dependencies

- **SD / SPI** — Arduino SD card library
- **FreeRTOS** — `logMutex` (`SemaphoreHandle_t`) must be created by the application before any SD operations
- **RockblockFunction** — `Table`, `TableEntry`, `add_entry()`, `send_table()`, etc. (see [`RockblockFunction/`](../RockblockFunction/))

> `logMutex` is declared `extern` in `SdFunction.h` — the application must call `logMutex = xSemaphoreCreateMutex()` during setup.

---

## Public API

### Initialization

#### `bool initSDCard()`
Mounts the SD card on `SD_CS_PIN` and prints the card size over `Serial`. If `sensor_log.csv` does not yet exist, creates it and writes the CSV header row (`timestamp_ms,temp,pressure`). Returns `false` on any failure.

#### `bool initRockblockBuffer()`
Allocates the internal `rockblockTable` via `new_table()` if it has not already been created. Returns `false` if allocation fails.

---

### Data Ingestion

#### `void writeDataToBuffer(const char* name, float value)`
The main data ingestion point. Accepts a sensor name and its reading. On each call it:

1. **RockBLOCK Check** — If `rockblockTable` exists and has budget remaining (serialized size + one `TableEntry` ≤ 338 bytes[^1]), appends a `TableEntry` timestamped with `millis()`. The unrecognized field is set to its sentinel (`InvalidTemp` or `InvalidPressure`).
2. **CSV path** — Formats a row as `timestamp_ms,<value or blank>` and appends it to `LogBuffer`. If the buffer would overflow, automatically calls `LogWriteBuffer()` to flush to SD before appending.

Both paths acquire `logMutex` independently. Unrecognized `name` values are silently ignored.

---

### Flushing

#### `bool LogWriteBuffer()`
Acquires `logMutex`, appends all pending bytes in `LogBuffer` to `sensor_log.csv` using `FILE_APPEND`, then clears the buffer. Returns `false` if the mutex cannot be acquired, the file cannot be opened, or a partial write is detected.

#### `bool sendRockblockBuffer()`
If `ROCKBLOCK_SEND_INTERVAL_MS` has elapsed since the last send and the table has at least one entry, calls `send_table()`, frees the old table, and allocates a fresh one. Returns `true` if the interval has not yet elapsed or if the send+reinit succeeds; returns `false` if the new table allocation fails.

---

## Expected Usage Flow

```cpp
// 1. Application setup
logMutex = xSemaphoreCreateMutex();
initSDCard();
initRockblockBuffer();

// 2. As sensor data arrives (from any task/ISR-safe context)
writeDataToBuffer("temp", readTemp());
writeDataToBuffer("pressure", readPressure());

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
| `LogBuffer`            | `char[65536]`  | RAM buffer for pending CSV rows                  |
| `logBufferlen`         | `size_t`       | Number of valid bytes currently in `LogBuffer`   |
| `sdReady`              | `bool`         | SD mount status (set externally or by app)       |
| `lastWriteTime`        | `uint32_t`     | `millis()` timestamp of last SD write            |
| `rockblockTable`       | `Table*`       | Active RockBLOCK entry buffer (module-internal)  |
| `lastRockblockSendTime`| `uint32_t`     | `millis()` timestamp of last satellite send      |

[^1]: Max size is 340 bytes, leaves 2 out bytes for checksum
