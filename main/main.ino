/* ---------------------------- library includes --------------------------- */

#include <sstream>

#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP3XX.h>
#include <freertos/FreeRTOS.h>

#include "src/SdFunction/SdFunction.h"
#include "src/RockblockFunction/RockblockFunction.h"
#include "src/aht30Lib/driver_aht30.h"
#include "src/aht30Lib/driver_aht30_basic.h"
#include "src/aht30Function/aht30Function.h"
#include "src/BluetoothFunction/BluetoothFunction.h"
#include "src/PIDHeatController/PIDHeatController.h"
#include "src/PWMController/PWMController.h"
#include "src/INA228/INA.h"
#include "src/Sensors.h"

/* ----------------------------------- IO ----------------------------------- */

SemaphoreHandle_t logMutex = NULL;
Adafruit_MPU6050 mpu;

Adafruit_BMP3XX bmp1;
Adafruit_BMP3XX bmp2;

uint64_t lastPID = 0;
uint64_t lastTime = 0;
float dutyCycle = 0.0f;

float temp1sec;

void readCore();
void writeCore();

constexpr BaseType_t READ_CORE_ID = 1;
constexpr BaseType_t WRITE_CORE_ID = 0;

void sensorTask(void*) {
    readCore();
}

void sdWriteTask(void*) {
    writeCore();
}

/* ----------------------------- core processes ----------------------------- */

void readCore() {
    while (true) {      

        // AHT30 sensor 1
        std::tuple<float, uint8_t, bool> sensorData = readAht30();
        float temperature = std::get<0>(sensorData);
        uint8_t humidity = std::get<1>(sensorData);
        bool success = std::get<2>(sensorData);

        if (success) {
            writeDataToBuffer("TempIns", temperature);
            writeDataToBuffer("Humidity", (float)humidity);
            
        }

        if (millis() - lastPID < 1000) {
            temp1sec = temperature;
        }
        
        // MPU6050 sensor 
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        writeDataToBuffer("MPUTemp", temp.temperature);
        writeDataToBuffer("AccX", a.acceleration.x);
        writeDataToBuffer("AccY", a.acceleration.y);
        writeDataToBuffer("AccZ", a.acceleration.z);

        // BMP390 sensor 1
        // bmp3_data bmp_data = Temp_Presure_Write_To_SD();
        
        // if (bmp_data.success) {
        //     writeDataToBuffer("BMP390_temperature", (float)bmp_data.temperature);
        //     writeDataToBuffer("BMP390_pressure", (float)bmp_data.pressure);
        // }

        // INA 228 High Voltage
        std::tuple<float, float> ina228Data = ReadINA228();
        float current = std::get<0>(ina228Data);
        float voltage = std::get<1>(ina228Data);
        writeDataToBuffer("hCurrent", current);
        writeDataToBuffer("hVoltage", voltage);



        delay(100);
    }
}

void writeCore() {
    while (true) {

        /* ------------------------------ SD Card Write ----------------------------- */

        uint32_t now = millis();
        if (now - lastWriteTime >= WRITE_INTERVAL_MS) {
            if (!LogWriteBuffer()) {
                Serial.println("Failed to write log buffer to SD");
            } else {
                if (!sendRockblockBuffer()) {
                    Serial.println("Failed to send rockblock buffer");
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

        if (SerialBT.available() > 0)
        {
            // Clear buffer

            String incoming = SerialBT.readStringUntil('\n');
            incoming.trim();

            int splitIndex = incoming.indexOf(' ');
            if (splitIndex > 0)
            {
                String command = incoming.substring(0, splitIndex);
                String value = incoming.substring(splitIndex + 1);
                value.trim();
                float parsed = value.toFloat();

                if (command == "kp")
                {
                    kp = parsed;
                    SerialBT.print("Updated kp: ");
                    SerialBT.println(kp);
                }
                else if (command == "ki")
                {
                    ki = parsed;
                    SerialBT.print("Updated ki: ");
                    SerialBT.println(ki);
                }
                else if (command == "kd")
                {
                    kd = parsed;
                    SerialBT.print("Updated kd: ");
                    SerialBT.println(kd);
                }
                else if (command == "target")
                {
                    targetTemperature = parsed;
                    SerialBT.print("Updated target temperature: ");
                    SerialBT.println(targetTemperature);
                }
                else if (command == "duty")
                {
                    dutyCycle = (int)parsed;
                    dutyCycle = constrain(dutyCycle, 0, 255);
                    SerialBT.print("Updated duty cycle: ");
                    SerialBT.println(dutyCycle);
                    ledcWrite(ledChannel, dutyCycle);
                }
                else if (command == "disable")
                {
                    btStop();
                    SerialBT.println("Bluetooth disabled");
                }
                else
                {
                    SerialBT.println("Unknown command");
                }
            }
            while (SerialBT.available() > 0)
                SerialBT.read();
        }

        delay(100); 
    }
}

/* ------------------------------ inital setup ------------------------------ */

void setup() {
    Serial.begin(115200);
    Serial.println("Hello, World!");
    
    while(!Serial) {
        delay(100);
    }

    // randomSeed((uint32_t)esp_random());

    /* ---------------------------------- inits --------------------------------- */

    // Initialize AHT30 Temperature sensor

    if (aht30_basic_init() != 0) {
        Serial.println("Failed to initialize AHT30 sensor");
        while (true) {
            delay(1000);
        }
    }
    Serial.println("AHT30 Initialized");

    // Initialize MPU6050 sensor
    if(!mpu.begin()) {
        Serial.println("Failed to initialize MPU6050 sensor");
        while (true) {
            delay(1000);
        }
    }
    Serial.println("BMP6050 Initialized");

    // Initialize BMP390 Pressure sensor
    if (!bmp1.begin_I2C()) {
        Serial.println("Could not find a valid BMP sensor #1, check wiring!");
        while (1)
            ;
    }
    Serial.println("BMP1 Initialized");

    if (!bmp2.begin_I2C(0x76)) {
        Serial.println("Could not find a valid BMP sensor #2, check wiring!");
        while (1)
            ;
    }
    Serial.println("BMP2 Initialized");

    // Initialize Mutex

    logMutex = xSemaphoreCreateMutex(); 
    if (logMutex == NULL) {
        Serial.println("Failed to create mutex");
        while (true) {
            delay(1000);
        }
    }


    // Initialize SD card
    
    sdReady = initSDCard();
    if (!sdReady) {
        Serial.println("Failed to initialize SD card");
        while (true) {
            delay(1000);
        }
    }

    // Initialize rockblock buffer

    if (!initRockblockBuffer()) {
        Serial.println("Failed to initialize rockblock buffer");
        while (true) {
            delay(1000);
        }
    }

    // Initialize INA228 sensor
    initializeINA228();


    // Initialize PID controller
    PWMSetup();


    // Initialize Bluetooth after blocking setup work to avoid early session drops.
    
    initBluetooth();

    /* --------------------------- Create pinned tasks -------------------------- */

    xTaskCreatePinnedToCore(
        sensorTask,
        "SensorDataTask",
        4096,
        NULL,
        1,
        NULL,
        READ_CORE_ID
    );
    
    xTaskCreatePinnedToCore(
        sdWriteTask,
        "SDWriteTask",
        4096,
        NULL,
        1,
        NULL,
        WRITE_CORE_ID
    );
}

// don't use
void loop() {}
