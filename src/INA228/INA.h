#pragma once   

#include <Arduino.h>
#include <BluetoothSerial.h>


extern BluetoothSerial SerialBT;

void initializeINA228();
std::tuple<float, float> ReadINA228();
void printINA228();