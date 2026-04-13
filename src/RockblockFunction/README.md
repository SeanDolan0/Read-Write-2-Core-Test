# RockblockFunction

**Files:** `RockblockFunction.h` · `RockblockFunction.cpp`

## Overview

Maintains an in-memory table of sensor snapshots and serializes it into a compact binary payload suitable for transmission over the [Iridium Short Burst Data (SBD)](https://www.iridium.com/services/iridium-sbd/) protocol. Entries are batched until the serialized payload approaches the 340-byte budget, at which point the table is sent and reset automatically.


---

## Data Types

### `TableEntry` *(packed)*

A single sensor snapshot. The struct is `__attribute__((packed))` to eliminate padding and keep the serialized footprint predictable.

| Field      | Type       | Description                          |
|------------|------------|--------------------------------------|
| `time`     | `uint64_t` | Milliseconds since device boot (uptime; e.g., from `millis()`) |
| `temp`     | `float`    | Temperature reading; `InvalidTemp` if unavailable |
| `pressure` | `float`    | Pressure reading; `InvalidPressure` if unavailable |

**Sentinel values**

| Constant          | Value    | Meaning                        |
|-------------------|----------|--------------------------------|
| `InvalidTemp`     | `-512.0f`| Temperature sensor unavailable |
| `InvalidPressure` | `-1.0f`  | Pressure sensor unavailable    |


---

### `Table`

A heap-allocated, dynamically-resizable array of `TableEntry` values.

| Field      | Type           | Description                              |
|------------|----------------|------------------------------------------|
| `size`     | `unsigned short` | Number of entries currently stored     |
| `capacity` | `unsigned short` | Allocated entry slots (grows x2 on overflow) |
| `entries`  | `TableEntry*`  | Heap-allocated array of entries          |

Initial capacity: **64 entries**. Capacity doubles automatically when entries are added past the current limit.

---

### `SerializedTable`

A `typedef void*` pointing to a flat binary buffer. Layout:

```
[ size (2 bytes) | capacity (2 bytes) | entries (size × 16 bytes) ]
```

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

#### `Table* add_sensor_data(Table* t, uint64_t time, float temp, float pressure)`
The primary entry point for recording sensor readings. Handles the full lifecycle:
1. Calls `checkTable(t)` — creates a new table if `t == NULL`, or flushes and resets if the next entry would exceed 340 bytes.
2. Appends a `TableEntry` with the given values.
3. Returns the (possibly new) table pointer.

> **Always reassign the return value:** `t = add_sensor_data(t, time, temp, pressure);`

#### `void add_entry(Table* t, TableEntry e)`
Low-level append. Doubles capacity via `realloc()` if `size >= capacity`. Does not check size limits — use `add_sensor_data()` for automatic budget management.

#### `Table* checkTable(Table* t)`
- If `t == NULL`: allocates and returns a fresh table.
- If the next entry would push serialized size past 340 bytes: calls `send_table(t)`, frees `t`, and returns a fresh table.
- Otherwise: returns `t` unchanged.

---

### Serialization

#### `SerializedTable serialize_table(Table* t)`
Serializes the table to a flat binary buffer (see layout above). Returns `NULL` on allocation failure.
#### `Table* deserialize_table(SerializedTable t)`
Reconstructs a `Table` from a previously serialized buffer. Allocates a new `Table` and copies entries from the buffer. 

#### `size_t table_memsize(Table* t)`
Returns the byte length of the serialized representation: `2 * sizeof(unsigned short) + t->size * sizeof(TableEntry)`.

---

### Transmission

#### `void send_table(Table* t)`
Seals and serializes the table, then prints the entry count and byte size over `Serial`. The Iridium SBD send call is stubbed out:

```cpp
int status = IridiumModem.sendSBDBinary((uint8_t *)st, size);
```
---

## Usage Example

```cpp
Table *t = NULL;

// In your sensor loop:
t = add_sensor_data(t, millis(), readTemp(), readPressure());

// At the end of a mission or on demand:
if (t && t->size > 0) {
    send_table(t);
    free_table(t);
    t = NULL;
}
```

Use `InvalidTemp` or `InvalidPressure` when a sensor is offline:

```cpp
t = add_sensor_data(t, millis(), InvalidTemp, readPressure());
```