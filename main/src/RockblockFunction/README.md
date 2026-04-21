# RockblockFunction

**Files:** `RockblockFunction.h` · `RockblockFunction.cpp`

## Overview

Maintains an in-memory table of sensor snapshots and serializes it into a compact binary payload suitable for transmission over the [Iridium Short Burst Data (SBD)](https://www.iridium.com/services/iridium-sbd/) protocol. Entries are batched until the serialized payload approaches the 340-byte budget, at which point the table is sent and reset automatically.

In practice, entries are added directly via `add_entry()` from [`SdFunction`](../SdFunction/) rather than through `add_sensor_data()`.

---

## Data Types

### `TableEntry` *(packed)*

A single sensor reading. The struct is `__attribute__((packed))` to eliminate padding and keep the serialized footprint predictable.

| Field  | Type             | Description                                          |
|--------|------------------|------------------------------------------------------|
| `time` | `uint64_t`       | Milliseconds since device boot (uptime; from `millis()`) |
| `type` | `SensorDataType` | Which sensor produced this reading (see `Sensors.h`) |
| `data` | `float`          | The sensor value                                     |

Each entry is **16 bytes** (8 + 4 + 4). Sentinel / invalid values for each sensor type are defined in [`Sensors.h`](../Sensors.h).

---

### `Table`

A heap-allocated, dynamically-resizable array of `TableEntry` values.

| Field      | Type             | Description                                        |
|------------|------------------|----------------------------------------------------|
| `size`     | `unsigned short` | Number of entries currently stored                 |
| `capacity` | `unsigned short` | Allocated entry slots (doubles on overflow)        |
| `entries`  | `TableEntry*`    | Heap-allocated array of entries                    |

Initial capacity: **64 entries**. Capacity doubles automatically when entries are added past the current limit.

---

### `SerializedTable`

A `typedef void*` pointing to a flat binary buffer. Layout:

```
[ size (2 bytes) | capacity (2 bytes) | entries (size × 16 bytes) ]
```

---

## Public API

### Table Lifecycle

#### `Table* new_table()`
Allocates and returns a new, empty `Table` with `capacity = 64`. Returns `NULL` on allocation failure.

#### `void free_table(Table* t)`
Frees both the `entries` array and the `Table` struct itself.

#### `void seal_table(Table* t)`
Shrinks the `entries` allocation to exactly `size` entries, eliminating unused capacity. Called automatically by `send_table()` before serialization.

---

### Adding Data

#### `void add_entry(Table* t, TableEntry e)`
Low-level append. Doubles capacity via `realloc()` if `size >= capacity`. Does not check the 340-byte size limit — callers should verify budget before calling (e.g. `table_memsize(t) + sizeof(TableEntry) + 2 <= 340`).

#### `Table* add_sensor_data(Table* t, uint64_t time, SensorDataType type, float data)` *(deprecated)*
Convenience wrapper that calls `checkTable(t)` then `add_entry()`. Returns the (possibly new) table pointer. Prefer managing the table directly via `add_entry()` and `checkTable()`.

> **Always reassign the return value:** `t = add_sensor_data(t, time, type, value);`

#### `Table* checkTable(Table* t)`
- If `t == NULL`: allocates and returns a fresh table.
- If the next entry would push the serialized size past 340 bytes: calls `send_table(t)`, frees `t`, and returns a fresh table.
- Otherwise: returns `t` unchanged.

---

### Serialization

#### `SerializedTable serialize_table(Table* t)`
Serializes the table to a flat binary buffer (see layout above). Returns `NULL` on allocation failure.

#### `Table* deserialize_table(SerializedTable t)`
Reconstructs a `Table` from a previously serialized buffer. Intended for testing only.

#### `size_t table_memsize(Table* t)`
Returns the byte length of the serialized representation: `2 * sizeof(unsigned short) + t->size * sizeof(TableEntry)`.

---

### Transmission

#### `void send_table(Table* t)`
Seals and serializes the table, then prints the entry count and byte size over `Serial`. The Iridium SBD send call is currently stubbed out:

```cpp
// int status = IridiumModem.sendSBDBinary((uint8_t *)st, size);
```

---

## Usage Example

```cpp
#include "Sensors.h"

Table *t = NULL;

// Add a reading directly:
if (table_memsize(t) + sizeof(TableEntry) + 2 <= 340) {
    add_entry(t, (TableEntry){ .time = millis(), .type = TempIns, .data = readTemp() });
}

// At the end of a mission or on demand:
if (t && t->size > 0) {
    send_table(t);
    free_table(t);
    t = NULL;
}
```