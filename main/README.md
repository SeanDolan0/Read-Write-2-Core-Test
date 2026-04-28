# Sokoke — Source Modules

| Module | Description |
|--------|-------------|
| [`RockblockFunction/`](RockblockFunction/README.md) | Iridium SBD modem init, table buffer, and satellite transmission |
| [`SdFunction/`](SdFunction/README.md) | CSV logging to SD card (SdFat) with RockBLOCK integration |
| [`bmp_function/`](bmp_function/README.md) | BMP390 barometric pressure + temperature sensor |
| [`aht30Function/`](aht30Function/README.md) | AHT30 temperature + humidity sensor |
| [`ina_function/`](ina_function/README.md) | INA228 / INA219 voltage and current monitors |
| [`gyro_function/`](gyro_function/README.md) | FXOS8700 + FXAS21002C IMU with Madgwick orientation filter |
| [`mcp_function/`](mcp_function/README.md) | MCP9808 precision temperature sensor |
| [`BluetoothFunction/`](BluetoothFunction/README.md) | ESP32 Classic Bluetooth Serial (SPP) setup |
| [`PWMController/`](PWMController/README.md) | MOSFET PWM control via ESP32 LEDC |
| [`PIDHeatController/`](PIDHeatController/README.md) | Discrete-time PID controller for heater/fan output |
| [`log_wrapper/`](log_wrapper/README.md) | Unified Serial + Bluetooth + SD debug logging |
| `SensorInit.cpp` / `sensorInit.h` | Sensor object declarations and retry-based initialization |
| `Sensors.h` | `SensorDataType` enum, sensor alive flags, and PID variable externs |
