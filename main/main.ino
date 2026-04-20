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
#include "src/Sensors.h"

/* ----------------------------------- IO ----------------------------------- */

SemaphoreHandle_t logMutex = NULL;

Adafruit_BMP3XX bmp_outside;
Adafruit_BMP3XX bmp_inside;
bool bmp_outside_alive;
bool bmp_inside_alive;

Adafruit_FXOS8700 fxos = Adafruit_FXOS8700(0x1F);
Adafruit_FXAS21002C fxas = Adafruit_FXAS21002C(0x0021002C);
bool fxos_fxas_alive;
Madgwick madgwick;
float mag_offsets[3] = { 0.0, 0.0, 0.0 };

Adafruit_MCP9808 mcp = Adafruit_MCP9808();
bool mcp_alive;

Adafruit_AHTX0 aht;
bool aht_alive;

// TODO: make these correspond correctly
Adafruit_INA228 ina_low = Adafruit_INA228();
bool ina_low_alive;
Adafruit_INA219 ina_high;
bool ina_high_alive;

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

void sensorTask(void *) {
  readCore();
}

void sdWriteTask(void *) {
  writeCore();
}

/* ----------------------------- core processes ----------------------------- */

void readCore() {
  while (true) {

    // AHT30 sensor 1
    AHT_Data_Return sensorData = readAht30();
    if (sensorData.read_flag) {
      lineoutPrintf("Humidity: %f\n", sensorData.humidity);
      lineoutPrintf("Temperature: %f\n", sensorData.temperature);
    } else {
      lineout("Could not read AHT30 data");
    }
    // float temperature = std::get<0>(sensorData);
    // uint8_t humidity = std::get<1>(sensorData);
    // bool success = std::get<2>(sensorData);

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
      } else {
        if (!sendRockblockBuffer()) {
          lineout("Failed to send rockblock buffer");
        }
        lastWriteTime = now;
      }
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
  // Serial.printf("Attempt %d to initialize FXOS8700\n", current_attempt);

  if (!fxos.begin()) {
    return attempt_init_fxos8700(current_attempt + 1);
  }
  return 1;
}
int attempt_init_fxas21002(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  Serial.printf("Attempt %d to initialize FXAS21002\n", current_attempt);

  if (!fxas.begin()) {
    return attempt_init_fxas21002(current_attempt + 1);
  }
  return 1;
}
int attempt_init_bmp390(Adafruit_BMP3XX *bmp, int address, int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  Serial.printf("Attempt %d to initialize BMP390 on I2C Address 0x%x\n", current_attempt, address);

  if (!bmp->begin_I2C(address)) {
    return attempt_init_bmp390(bmp, address, current_attempt + 1);
  }
  return 1;
}
int attempt_init_mcp9808(int current_attempt) {
  if (current_attempt > MAX_INIT_ATTEMPTS) {
    return 0;
  }
  Serial.printf("Attempt %d to initialize MCP9808\n", current_attempt);

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
  Serial.printf("Attempt %d to initialize mutex\n", current_attempt);

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
  Serial.printf("Attempt %d to initialize SD Card Reader\n", current_attempt);

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
  Serial.printf("Attempt %d to initialize INA228 on I2C Address 0x%x\n", current_attempt, address);

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
  Serial.printf("Attempt %d to initialize INA219 on I2C Address 0x%x\n", current_attempt, address);

  *ina = Adafruit_INA219(address);
  if (!ina->begin()) {
    return attempt_init_ina219(ina, address, current_attempt + 1);
  }
  return 1;
}

void setup() {
  Serial.begin(115200);
  initBluetooth();

  lineout("ESP32 Started");

  while (!Serial) {
    delay(100);
  }

  // Initialize Mutex
  if (attempt_init_mutex()) {
    Serial.println("Mutex Initialized\n");
  } else {
    Serial.println("Failed to create mutex\n");
    ESP.restart();
  }

  // // Initialize SD card
  if (sdReady = attempt_init_sdreader()) {
    Serial.println("SD Card Reader Initialized\n");
  } else {
    Serial.println("Failed to initialize SD Card Reader\n");
  }

  // // Initialize rockblock buffer
  // if (!initRockblockBuffer()) {
  //     Serial.println("Failed to initialize rockblock buffer");
  //     while (true) {
  //         delay(1000);
  //     }
  // }

  // randomSeed((uint32_t)esp_random());

  /* ---------------------------------- inits --------------------------------- */

  // Initialize AHT30 Temperature sensor
  if (aht_alive = attempt_init_aht30()) {
    Serial.println("AHT30 Initialized\n");
  } else {
    Serial.println("Failed to initialize AHT30 sensor\n");
  }

  // // Initialize FXOS8700/FXAS21002 sensor
  if (fxos_fxas_alive = (attempt_init_fxos8700() && attempt_init_fxas21002())) {
    Serial.println("FXOS8700/FXAS21002 Initialized\n");
  } else {
    Serial.println("Failed to initialize FXOS8700/FXAS21002 sensor\n");
    madgwick.begin(100);
  }

  // Initialize BMP390 Pressure sensor
  if (bmp_outside_alive = attempt_init_bmp390(&bmp_outside, 0x77)) { // outside temp/pres
      Serial.println("Outside BMP390 on I2C Address 0x77 Initialized\n");
  } else {
      Serial.println("Could not find a valid outside BMP sensor, check wiring!\n");
  }

  if (bmp_inside_alive = attempt_init_bmp390(&bmp_inside, 0x76)) {  // inside temp/pres
    Serial.println("Inside BMP390 on I2C Address 0x76 Initialized\n");
  } else {
    Serial.println("Could not find a valid inside BMP sensor, check wiring!\n");
  }

  if (mcp_alive = attempt_init_mcp9808()) {
      Serial.println("MCP9808 Initialized\n");
  } else {
      Serial.println("Failed to initialize MCP9808 sensor\n");
  }

  if (ina_low_alive = attempt_init_ina228(&ina_low, 0x41)) {
    Serial.println("Low Voltage INA228 Initialized\n");
  } else {
    Serial.println("Failed to initialize Low Voltage INA228 sensor\n");
  }
  if (ina_high_alive = attempt_init_ina219(&ina_high, 0x40)) {
    Serial.println("High Voltage INA219 Initialized\n");
  } else {
    Serial.println("Failed to initialize High Voltage INA219 sensor\n");
  }

  // Initialize PID controller
  PWMSetup();

  // Initialize Bluetooth after blocking setup work to avoid early session drops.

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
