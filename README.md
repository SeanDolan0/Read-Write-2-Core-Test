# [Project Name]

> [One-sentence description of what this project does and its goal.]

---

## Table of Contents

- [Overview](#overview)
- [Hardware](#hardware)
- [Getting Started](#getting-started)
- [Architecture](#architecture)
- [Source Modules](#source-modules)
- [Bluetooth Commands](#bluetooth-commands)
- [Configuration](#configuration)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

[Describe the project in 2–4 sentences. What problem does it solve? What platform does it run on? What are its key capabilities (data logging, satellite transmission, thermal control, etc.)?]

---

## Hardware

| Component | Part Number | Role |
|-----------|-------------|------|
| Microcontroller | ESP32 (DOIT DevKit v1) | Dual-core main processor |
| Satellite modem | RockBLOCK 9603 | Iridium SBD uplink |
| SD card reader | — | CSV data logging |
| Pressure sensor (inside) | BMP390 (0x76) | Internal enclosure pressure/temp |
| Pressure sensor (outside) | BMP390 (0x77) | Ambient pressure/temp |
| Humidity sensor | AHT30 | Relative humidity + temperature |
| Precision temp sensor | MCP9808 (0x18) | High-accuracy internal temperature |
| IMU | FXOS8700 + FXAS21002C | Orientation + linear acceleration |
| Power monitor (low voltage) | INA228 (0x41) | Bus voltage and current |
| Power monitor (high voltage) | INA219 (0x40) | Bus voltage and current |
| Heater driver | FQP30N06L MOSFET | PWM-driven resistive heater |
| Fan driver | FQP30N06L MOSFET | PWM-driven cooling fan |

[Add or remove rows as needed.]

---

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- ESP32 Arduino framework
- All hardware connected and wired (see [Hardware](#hardware))

### Build & Upload

```bash
# Install dependencies and build
pio run

# Upload to connected ESP32
pio run --target upload

# Open serial monitor
pio device monitor --baud 115200
```

### First Boot

On first boot the firmware will:
1. Initialize Bluetooth as **"ESP32-Serial"** — pair from your device.
2. Initialize all sensors with up to 10 retries each.
3. Create `sensor_log.csv` on the SD card if it does not exist.
4. Initialize the Iridium modem.
5. Spawn two FreeRTOS tasks pinned to separate cores.

---

## Architecture

The firmware uses a dual-core FreeRTOS architecture:

| Core | Task | Responsibility |
|------|------|----------------|
| Core 1 | `SensorDataTask` | Reads all sensors every 100 ms and writes to the log buffer |
| Core 0 | `SDWriteTask` | Flushes buffer to SD, sends satellite data, runs PID, handles BT commands |

A shared FreeRTOS mutex (`logMutex`) serializes all SD card and debug log access between the two tasks.

---

## Source Modules

See [`main/README.md`](main/README.md) for the full module list with links to individual module documentation.

| Module | Description |
|--------|-------------|
| [`RockblockFunction/`](main/src/RockblockFunction/README.md) | Iridium SBD modem init, table buffer, and satellite transmission |
| [`SdFunction/`](main/src/SdFunction/README.md) | CSV logging to SD card with RockBLOCK integration |
| [`bmp_function/`](main/src/bmp_function/README.md) | BMP390 pressure + temperature |
| [`aht30Function/`](main/src/aht30Function/README.md) | AHT30 humidity + temperature |
| [`ina_function/`](main/src/ina_function/README.md) | INA228 / INA219 power monitors |
| [`gyro_function/`](main/src/gyro_function/README.md) | IMU orientation via Madgwick filter |
| [`mcp_function/`](main/src/mcp_function/README.md) | MCP9808 precision temperature |
| [`BluetoothFunction/`](main/src/BluetoothFunction/README.md) | Classic Bluetooth Serial (SPP) |
| [`PWMController/`](main/src/PWMController/README.md) | LEDC PWM for heater and fan |
| [`PIDHeatController/`](main/src/PIDHeatController/README.md) | Discrete-time PID for thermal control |
| [`log_wrapper/`](main/src/log_wrapper/README.md) | Unified Serial + BT + SD debug logging |

---

## Bluetooth Commands

Connect to **"ESP32-Serial"** from any Bluetooth terminal app and send commands as plain text lines.

| Command          | Example         | Description                                    |
|------------------|-----------------|------------------------------------------------|
| `kp <value>`     | `kp 10.0`       | Update PID proportional gain                   |
| `ki <value>`     | `ki 0.5`        | Update PID integral gain                       |
| `kd <value>`     | `kd 1.5`        | Update PID derivative gain                     |
| `target <value>` | `target 30.0`   | Set heater target temperature (°C)             |
| `cycle <value>`  | `cycle 128`     | Manually set PWM duty cycle (0–255)            |
| `debugbt 1`      | `debugbt 1`     | Enable SPP event logging and connection status |
| `debugbt 0`      | `debugbt 0`     | Disable SPP event logging                      |
| `initaht`        | `initaht`       | Manually re-initialize AHT30 sensor            |
| `status`         | `status`        | Print alive status of all sensors              |
| `fullreset`      | `fullreset`     | Restart the ESP32                              |

---

## Configuration

Key constants that may need tuning for your deployment:

| Location | Constant | Default | Description |
|----------|----------|---------|-------------|
| `SdFunction.cpp` | `WRITE_INTERVAL_MS` | `30000` | SD flush interval (ms) |
| `SdFunction.cpp` | `ROCKBLOCK_SEND_INTERVAL_MS` | `300000` | Satellite send interval (ms) |
| `SdFunction.cpp` | `CSV_LOG_BUFFER_SIZE` | `49152` | RAM log buffer size (bytes) |
| `PIDHeatController.cpp` | `kp`, `ki`, `kd` | `8.0`, `0.0`, `2.0` | PID gains |
| `PIDHeatController.cpp` | `targetTemperature` | `35.0` | Heater target (°C) |
| `main.ino` | `IRIDIUM_RX_PIN`, `IRIDIUM_TX_PIN` | `16`, `17` | RockBLOCK UART pins |
| `SensorInit.cpp` | `MAX_INIT_ATTEMPTS` | `10` | Sensor init retries |

---

## Contributing

[Describe how contributors should submit changes, run tests, follow coding style, etc.]

---

## License

[Specify the license or leave blank until decided.]
