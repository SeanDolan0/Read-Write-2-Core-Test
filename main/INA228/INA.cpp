
#include <Arduino.h>
#include <Adafruit_INA228.h>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <numeric>
#include <iostream>
#include "src/INA228/INA.h"

std::vector<float> VbufferVec(256);
std::vector<float> AbufferVec(256);

void initializeINA228() {
if (!ina.begin()) {
    SerialBT.println("Couldn't find INA228 chip");
    while (1)
      ;
  }
  SerialBT.println("Found INA228 chip");
  // set shunt resistance and max current
  ina.setShunt(0.015, 10.0);

  ina.setAveragingCount(INA228_COUNT_16);
  uint16_t counts[] = {1, 4, 16, 64, 128, 256, 512, 1024};
  SerialBT.print("Averaging counts: ");
  SerialBT.println(counts[ina.getAveragingCount()]);

  // set the time over which to measure the current and bus voltage
  ina.setVoltageConversionTime(INA228_TIME_150_us);
  SerialBT.print("Voltage conversion time: ");
  switch (ina.getVoltageConversionTime()) {
  case INA228_TIME_50_us:
    SerialBT.print("50");
    break;
  case INA228_TIME_84_us:
    SerialBT.print("84");
    break;
  case INA228_TIME_150_us:
    SerialBT.print("150");
    break;
  case INA228_TIME_280_us:
    SerialBT.print("280");
    break;
  case INA228_TIME_540_us:
    SerialBT.print("540");
    break;
  case INA228_TIME_1052_us:
    SerialBT.print("1052");
    break;
  case INA228_TIME_2074_us:
    SerialBT.print("2074");
    break;
  case INA228_TIME_4120_us:
    SerialBT.print("4120");
    break;
  }
  SerialBT.println(" uS");

  ina.setCurrentConversionTime(INA228_TIME_280_us);
  SerialBT.print("Current conversion time: ");
  switch (ina.getCurrentConversionTime()) {
  case INA228_TIME_50_us:
    SerialBT.print("50");
    break;
  case INA228_TIME_84_us:
    SerialBT.print("84");
    break;
  case INA228_TIME_150_us:
    SerialBT.print("150");
    break;
  case INA228_TIME_280_us:
    SerialBT.print("280");
    break;
  case INA228_TIME_540_us:
    SerialBT.print("540");
    break;
  case INA228_TIME_1052_us:
    SerialBT.print("1052");
    break;
  case INA228_TIME_2074_us:
    SerialBT.print("2074");
    break;
  case INA228_TIME_4120_us:
    SerialBT.print("4120");
    break;
  }
  SerialBT.println(" uS");
}

INA_Data_Return ReadINA228() {
    SerialBT.print("Bus Voltage: ");
    float busVoltage = ina.getBusVoltage_V();
    SerialBT.print(busVoltage, 3);
    SerialBT.print(" V \nShunt Voltage: ");
    float current = ina.getCurrent_mA();
    SerialBT.print(current, 3);
    SerialBT.println(" mA");
    return (INA_Data_Return){
      .current = current,
      .busVoltage = busVoltage,
    };
}
