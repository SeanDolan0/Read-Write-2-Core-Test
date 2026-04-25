#include "src/Sensors.h"
#include <Arduino.h>


float kp = 20.0f;
float ki = 1.0f;
float kd = 5.0f;
float targetTemperature = 30.0f;
float maxVal = 255.0f;

float integral = 0;
float derivative = 0;
float last_error = 0;

float CalculatePID(float target, float current, float dt) {
  float error = target - current;
  integral += error * dt;
  derivative = (error - last_error) / dt;
  last_error = error;

  float output = kp * error + ki * integral + kd * derivative;
  return std::max(0.0f, std::min(output, maxVal));
}
