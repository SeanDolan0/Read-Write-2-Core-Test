/* ---------------------------- library includes --------------------------- */

#include <sstream>
#include <string>

#include <freertos/FreeRTOS.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_FXOS8700.h>
#include <Adafruit_FXAS21002C.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_INA228.h>
#include <Adafruit_INA219.h>
#include <MadgwickAHRS.h>

#include "src/SdFunction/SdFunction.h"
#include "src/RockblockFunction/RockblockFunction.h"
#include "src/aht30Function/aht30Function.h"
#include "src/BluetoothFunction/BluetoothFunction.h"
#include "src/PIDHeatController/PIDHeatController.h"
#include "src/PWMController/PWMController.h"
#include "src/log_wrapper/log_wrapper.h"
#include "src/gyro_function/gyro_function.h"
#include "src/bmp_function/bmp_function.h"
#include "src/mcp_function/mcp_function.h"
#include "src/ina_function/ina_function.h"
#include "src/Sensors.h"

/* ----------------------------------- IO ----------------------------------- */

SemaphoreHandle_t logMutex = NULL;

bool bmp_outside_alive;
bool bmp_inside_alive;
bool fxos_fxas_alive;
bool mcp_alive;
bool aht_alive;
bool ina_low_alive;
bool ina_high_alive;

Adafruit_FXOS8700 fxos = Adafruit_FXOS8700(0x1F);
Adafruit_FXAS21002C fxas = Adafruit_FXAS21002C(0x0021002C);
Madgwick madgwick;
float mag_offsets[3] = { 0.0, 0.0, 0.0 };

Adafruit_BMP3XX bmp_outside;
Adafruit_BMP3XX bmp_inside;

Adafruit_MCP9808 mcp = Adafruit_MCP9808();

Adafruit_AHTX0 aht;

// TODO: make these correspond correctly
Adafruit_INA228 ina_low = Adafruit_INA228();
Adafruit_INA219 ina_high;

uint64_t lastPID = 0;
uint64_t lastTime = 0;
float dutyCycle = 0.0f;

float temp1sec;

void readCore();
void writeCore();

constexpr BaseType_t READ_CORE_ID = 1;
constexpr BaseType_t WRITE_CORE_ID = 0;

int attempt_init_fxos8700(int current_attempt = 1);
int attempt_init_fxas21002(int current_attempt = 1);
int attempt_init_bmp390(Adafruit_BMP3XX *bmp, int address, int current_attempt = 1);
int attempt_init_mcp9808(int current_attempt = 1);
int attempt_init_aht30(int current_attempt = 1);
int attempt_init_mutex(int current_attempt = 1);
int attempt_init_sdreader(int current_attempt = 1);
int attempt_init_ina228(Adafruit_INA228 *ina, int address, int current_attempt = 1);
int attempt_init_ina219(Adafruit_INA219 *ina, int address, int current_attempt = 1);
int attempt_init_rockblock_buffer(int current_attempt = 1);

void sensorTask(void *) {
  readCore();
}

void sdWriteTask(void *) {
  writeCore();
}

/* ----------------------------- core processes ----------------------------- */

void readCore() {
  while (true) {

    // AHT30 sensor
    AHT_Data_Return aht_data = readAht30();
    if (aht_data.success) {
      lineoutPrintf("Humidity: %.2f%\n", aht_data.humidity);
      lineoutPrintf("Temperature: %.2f C\n", aht_data.temperature);

      writeDataToBuffer("AhtTemperature", aht_data.temperature);
      writeDataToBuffer("AhtHumidity", aht_data.humidity);
    } else {
      lineout("Could not read AHT30 data");
    }

    // FXOS8700 / FXAS21002C sensor
    GyroData gyro_data = read_fxos_fxas_gyro();
    if (gyro_data.success) {
      lineoutPrintf("Roll: %.2f\n", gyro_data.angle.roll);
      lineoutPrintf("Pitch: %.2f\n", gyro_data.angle.pitch);
      lineoutPrintf("Yaw: %.2f\n", gyro_data.angle.yaw);
      lineoutPrintf("Linear X Acceleration: %.2f m/s^2\n", gyro_data.linacc_x);

      writeDataToBuffer("GyroRoll", gyro_data.angle.roll);
      writeDataToBuffer("GyroPitch", gyro_data.angle.pitch);
      writeDataToBuffer("GyroYaw", gyro_data.angle.yaw);
      writeDataToBuffer("GyroLinAccX", gyro_data.linacc_x);
    } else {
      lineout("Could not read AHT30 data");
    }
    
    // bmp inside
    BmpData bmp_inside_data = read_bmp(&bmp_inside, bmp_inside_alive);
    if (bmp_inside_data.success) {
      lineoutPrintf("InsBMP Temperature: %.2f C\n", bmp_inside_data.temp);
      lineoutPrintf("InsBMP Pressure: %.2f Pa\n", bmp_inside_data.pressure);

      writeDataToBuffer("InsBmpTemp", bmp_inside_data.temp);
      writeDataToBuffer("InsBmpPress", bmp_inside_data.pressure);
    } else {
      lineout("Could not read Inside BMP390 data");
    }
    // bmp outside
    BmpData bmp_outside_data = read_bmp(&bmp_outside, bmp_outside_alive);
    if (bmp_outside_data.success) {
      lineoutPrintf("OutBMP Temperature: %.2f C\n", bmp_outside_data.temp);
      lineoutPrintf("OutBMP Pressure: %.2f Pa\n", bmp_outside_data.pressure);

      writeDataToBuffer("OutBmpTemp", bmp_outside_data.temp);
      writeDataToBuffer("OutBmpPress", bmp_outside_data.pressure);
    } else {
      lineout("Could not read Outside BMP390 data");
    }

    // mcp
    McpData mcp_data = read_mcp();
    if (mcp_data.success) {
      lineoutPrintf("MCP TempF: %f F\n", mcp_data.temp_f);
      lineoutPrintf("MCP TempC: %f C\n", mcp_data.temp_c);

      writeDataToBuffer("McpTempF", mcp_data.temp_f);
      writeDataToBuffer("McpTempC", mcp_data.temp_c);
    } else {
      lineout("Could not read MCP9808 data");
    }

    // ina low
    InaData ina_low_data = read_ina228(&ina_low, ina_low_alive);
    if (ina_low_data.success) {
      lineoutPrintf("LowINA228 Bus Voltage: %.2f V\n", ina_low_data.busVoltage);
      lineoutPrintf("LowINA228 Current: %.2f mA\n", ina_low_data.current);

      writeDataToBuffer("LowInaBusVolt", ina_low_data.busVoltage);
      writeDataToBuffer("LowInaCurrent", ina_low_data.current);
    } else {
      lineout("Could not read Low Voltage INA228");
    }
    // ina high
    InaData ina_high_data = read_ina219(&ina_high, ina_high_alive);
    if (ina_high_data.success) {
      lineoutPrintf("HighINA219 Bus Voltage: %.2f V\n", ina_high_data.busVoltage);
      lineoutPrintf("HighINA219 Current: %.2f mA\n", ina_high_data.current);

      writeDataToBuffer("HighInaBusVolt", ina_high_data.busVoltage);
      writeDataToBuffer("HighInaCurrent", ina_high_data.current);
    } else {
      lineout("Could not read High Voltage INA219");
    }

    lineout("");

    // if (success) {
    //     writeDataToBuffer("TempIns", temperature);
    //     writeDataToBuffer("Humidity", (float)humidity);
    // }

    // if (millis() - lastPID < 1000) {
    //     temp1sec = temperature;
    // }

    // MPU6050 sensor
    //% sensors_event_t a, g, temp;
    //% mpu.getEvent(&a, &g, &temp);
    //% writeDataToBuffer("MPUTemp", temp.temperature);
    //% writeDataToBuffer("AccX", a.acceleration.x);
    //% writeDataToBuffer("AccY", a.acceleration.y);
    //% writeDataToBuffer("AccZ", a.acceleration.z);

    // FXOS8700/FXAS21002 sensor
    // sensors_event_t a, m, g;
    // float mx = m.magnetic.x - mag_offsets[0];
    // float my = m.magnetic.y - mag_offsets[1];
    // float mz = m.magnetic.z - mag_offsets[2];
    // madgwick.update(g.gyro.x * 57.2958, g.gyro.y * 57.2958, g.gyro.z * 57.2958,
    //                 a.acceleration.x, a.acceleration.y, a.acceleration.z,
    //                 mx, my, mz);
    // float roll = filter.getRoll();
    // float pitch = filter.getPitch();
    // float yaw = filter.getYaw();
    // float lin_accel_x = a.acceleration.x - (sin(pitch * 0.0174) * 9.8);
    // writeDataToBuffer("AccX", lin_accel_x);

    // BMP390 sensor 1
    // bmp3_data bmp_data = Temp_Presure_Write_To_SD();

    // if (bmp_data.success) {
    //     writeDataToBuffer("BMP390_temperature", (float)bmp_data.temperature);
    //     writeDataToBuffer("BMP390_pressure", (float)bmp_data.pressure);
    // }

    // INA 228 High Voltage
    // std::tuple<float, float> ina228Data = ReadINA228();
    // float current = std::get<0>(ina228Data);
    // float voltage = std::get<1>(ina228Data);
    // writeDataToBuffer("hCurrent", current);
    // writeDataToBuffer("hVoltage", voltage);



    delay(100);
  }
}

void writeCore() {
  while (true) {

    /* ------------------------------ SD Card Write ----------------------------- */

    uint32_t now = millis();
    if (now - lastWriteTime >= WRITE_INTERVAL_MS) {
      if (!LogWriteBuffer()) {
        lineout("Failed to write log buffer to SD");
      }
      if (!sendRockblockBuffer()) {
        lineout("Failed to send rockblock buffer");
      }
      lastWriteTime = now;
    }
    if (now - lastPID >= 1000) {
      float pidOutput = CalculatePID(targetTemperature, temp1sec, 1.0f);
      (void)pidOutput;

      lastPID = now;
    }


    /* -------------------------------- Bluetooth ------------------------------- */

    if (SerialBT.available() > 0) {
      // Clear buffer

      String incoming = SerialBT.readStringUntil('\n');
      incoming.trim();

      int splitIndex = incoming.indexOf(' ');
      if (splitIndex > 0) {
        String command = incoming.substring(0, splitIndex);
        String value = incoming.substring(splitIndex + 1);
        value.trim();
        float parsed = value.toFloat();

        if (command == "ping") {
          lineout("pong");
        } else if (command == "kp") {
          kp = parsed;
          lineout("Updated kp: ");
          lineout(std::to_string(kp).c_str());
        } else if (command == "ki") {
          ki = parsed;
          lineout("Updated ki: ");
          lineout(std::to_string(ki).c_str());
        } else if (command == "kd") {
          kd = parsed;
          lineout("Updated kd: ");
          lineout(std::to_string(kd).c_str());
        } else if (command == "target") {
          targetTemperature = parsed;
          lineout("Updated target temperature: ");
          lineout(std::to_string(targetTemperature).c_str());
        } else if (command == "duty") {
          dutyCycle = (int)parsed;
          dutyCycle = constrain(dutyCycle, 0, 255);
          lineout("Updated duty cycle: ");
          lineout(std::to_string(dutyCycle).c_str());
          ledcWrite(ledChannel, dutyCycle);
        } else if (command == "disable") {
          btStop();
          lineout("Bluetooth disabled");
        } else {
          lineout("Unknown command");
        }
      } else {
        if (incoming == "initaht") {
          lineout("manually initializing aht30");
          attempt_init_aht30();
        } else if (incoming == "status") {
          lineout("Sensor statuses:");
          lineoutPrintf("Inside BMP390: %d\n", bmp_inside_alive);
          lineoutPrintf("Outside BMP390: %d\n", bmp_outside_alive);
          lineoutPrintf("FXOS8700/FXAS21002: %d\n", fxos_fxas_alive);
          lineoutPrintf("MCP9808: %d\n", mcp_alive);
          lineoutPrintf("AHT30: %d\n", aht_alive);
          lineoutPrintf("INA228 Low Voltage: %d\n", ina_low_alive);
          lineoutPrintf("INA219 High Voltage: %d\n", mcp_alive);
        }
      }
      while (SerialBT.available() > 0) SerialBT.read();
    }

    delay(100);
  }
}

/* ------------------------------ inital setup ------------------------------ */

#define MAX_INIT_ATTEMPTS 10

int attempt_init_fxos8700(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize FXOS8700\n", current_attempt);

  if (!fxos.begin()) {
    return attempt_init_fxos8700(current_attempt + 1);
  }
  return 1;
}
int attempt_init_fxas21002(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize FXAS21002\n", current_attempt);

  if (!fxas.begin()) {
    return attempt_init_fxas21002(current_attempt + 1);
  }
  madgwick.begin(100);
  return 1;
}
int attempt_init_bmp390(Adafruit_BMP3XX *bmp, int address, int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize BMP390 on I2C Address 0x%x\n", current_attempt, address);

  if (!bmp->begin_I2C(address)) {
    return attempt_init_bmp390(bmp, address, current_attempt + 1);
  }
  return 1;
}
int attempt_init_mcp9808(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize MCP9808\n", current_attempt);

  if (!mcp.begin(0x18)) {
    return attempt_init_mcp9808(current_attempt + 1);
  }
  mcp.setResolution(3);
  mcp.wake();
  return 1;
}
int attempt_init_aht30(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize AHT30\n", current_attempt);

  if (!aht.begin()) {
    return attempt_init_aht30(current_attempt + 1);
  }
  return 1;
}
int attempt_init_mutex(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize mutex\n", current_attempt);

  logMutex = xSemaphoreCreateMutex();
  if (logMutex == NULL) {
    return attempt_init_mutex(current_attempt + 1);
  }
  return 1;
}
int attempt_init_sdreader(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize SD Card Reader\n", current_attempt);

  if (!initSDCard()) {
    // delay(1000); // maybe give it some time
    return attempt_init_sdreader(current_attempt + 1);
  }
  return 1;
}
int attempt_init_ina228(Adafruit_INA228 *ina, int address, int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize INA228 on I2C Address 0x%x\n", current_attempt, address);

  if (!ina->begin(address)) {
    return attempt_init_ina228(ina, address, current_attempt + 1);
  }
  ina->setShunt(0.015, 10.0);
  return 1;
}
int attempt_init_ina219(Adafruit_INA219 *ina, int address, int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize INA219 on I2C Address 0x%x\n", current_attempt, address);

  *ina = Adafruit_INA219(address);
  if (!ina->begin()) {
    return attempt_init_ina219(ina, address, current_attempt + 1);
  }
  return 1;
}
int attempt_init_rockblock_buffer(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  lineoutPrintf("Attempt %d to initialize RockBlock\n", current_attempt);

  if (!initRockblockBuffer()) {
    return attempt_init_rockblock_buffer(current_attempt + 1);
  }
  return 1;
}

void setup() {
  Serial.begin(115200);
  initBluetooth();

  lineout("ESP32 Started");
  lineoutPrintf("Total heap available post-startup: %zu\n", ESP.getFreeHeap());

  while (!Serial) {
    delay(100);
  }

  // Initialize Mutex
  if (attempt_init_mutex()) {
    lineout("Mutex Initialized\n");
  } else {
    lineout("Failed to create mutex\n");
    ESP.restart();
  }

  // // Initialize SD card
  if (sdReady = attempt_init_sdreader()) {
    lineout("SD Card Reader Initialized\n");
  } else {
    lineout("Failed to initialize SD Card Reader\n");
  }

  // Initialize rockblock buffer
  if (attempt_init_rockblock_buffer()) {
    lineout("RockBlock Buffer Initialized\n");
  } else {
    lineout("RockBlock Buffer failed to initialize\n");
  }

  // randomSeed((uint32_t)esp_random());

  /* ---------------------------------- inits --------------------------------- */

  // Initialize AHT30 Temperature sensor
  if (aht_alive = attempt_init_aht30()) {
    lineout("AHT30 Initialized\n");
  } else {
    lineout("Failed to initialize AHT30 sensor\n");
  }

  // // Initialize FXOS8700/FXAS21002 sensor
  if (fxos_fxas_alive = (attempt_init_fxos8700() && attempt_init_fxas21002())) {
    lineout("FXOS8700/FXAS21002 Initialized\n");
  } else {
    lineout("Failed to initialize FXOS8700/FXAS21002 sensor\n");
    madgwick.begin(100);
  }

  // Initialize BMP390 Pressure sensor
  if (bmp_outside_alive = attempt_init_bmp390(&bmp_outside, 0x77)) { // outside temp/pres
      lineout("Outside BMP390 on I2C Address 0x77 Initialized\n");
  } else {
      lineout("Could not find a valid outside BMP sensor, check wiring!\n");
  }

  if (bmp_inside_alive = attempt_init_bmp390(&bmp_inside, 0x76)) {  // inside temp/pres
    lineout("Inside BMP390 on I2C Address 0x76 Initialized\n");
  } else {
    lineout("Could not find a valid inside BMP sensor, check wiring!\n");
  }

  if (mcp_alive = attempt_init_mcp9808()) {
      lineout("MCP9808 Initialized\n");
  } else {
      lineout("Failed to initialize MCP9808 sensor\n");
  }

  if (ina_low_alive = attempt_init_ina228(&ina_low, 0x41)) {
    lineout("Low Voltage INA228 Initialized\n");
  } else {
    lineout("Failed to initialize Low Voltage INA228 sensor\n");
  }
  if (ina_high_alive = attempt_init_ina219(&ina_high, 0x40)) {
    lineout("High Voltage INA219 Initialized\n");
  } else {
    lineout("Failed to initialize High Voltage INA219 sensor\n");
  }

  // Initialize PID controller
  PWMSetup();

  /* --------------------------- Create pinned tasks -------------------------- */

  xTaskCreatePinnedToCore(
    sensorTask,
    "SensorDataTask",
    4096,
    NULL,
    1,
    NULL,
    READ_CORE_ID);

  xTaskCreatePinnedToCore(
    sdWriteTask,
    "SDWriteTask",
    4096,
    NULL,
    1,
    NULL,
    WRITE_CORE_ID);
}

// don't use
void loop() {}
