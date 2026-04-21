#pragma once   

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <Adafruit_INA228.h>

typedef struct {
    float current;
    float busVoltage;
} INA_Data_Return;

void initializeINA228();
INA_Data_Return ReadINA228();
void printINA228();