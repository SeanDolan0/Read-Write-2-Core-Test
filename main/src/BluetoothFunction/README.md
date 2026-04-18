# BluetoothFunction

**Files:** `BluetoothFunction.h` · `BluetoothFunction.cpp`

## Overview

Sets up Classic Bluetooth Serial (SPP) on the ESP32 so that other modules can stream data wirelessly to a paired device. Once initialized, the global `SerialBT` object can be used anywhere in the project as a drop-in replacement for wired `Serial` output.

---

## Dependencies

- **BluetoothSerial** — ESP32 Arduino Bluetooth Serial library

---

## Exported Globals

| Symbol        | Type              | Description                                               |
|---------------|-------------------|-----------------------------------------------------------|
| `SerialBT`    | `BluetoothSerial` | Bluetooth serial port; used by other modules for output   |
| `isConnected` | `bool`            | `true` while a remote client is connected via SPP         |

---

## Public API

#### `void initBluetooth()`
Starts the Bluetooth stack with the device name **"ESP32-Serial"** in server (non-master) mode. Sets a 50 ms read timeout and registers the internal `connectionStatus` callback. Prints an error message to `Serial` and returns early if startup fails.

---

## Internal Callback

#### `void connectionStatus(esp_spp_cb_event_t event, esp_spp_cb_param_t* param)` *(static)*
Handles SPP connection/disconnection events:
- `ESP_SPP_SRV_OPEN_EVT` — sets `isConnected = true` and logs to `Serial`.
- `ESP_SPP_CLOSE_EVT` — sets `isConnected = false` and logs to `Serial`.

---

## Usage Example

```cpp
initBluetooth();

// In any task or loop:
if (isConnected) {
    SerialBT.printf("Temperature: %.2f C\n", temp);
}
```
