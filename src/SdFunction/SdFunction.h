#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t logMutex;

extern const uint32_t WRITE_INTERVAL_MS;

extern uint32_t lastWriteTime;
extern bool sdReady;

extern char csvLogBuffer[65536]; // 64KB buffer for log entries
extern size_t logBufferlen;

bool initSDCard();
bool LogWriteBuffer();
bool initRockblockBuffer();
bool sendRockblockBuffer();
void writeDataToBuffer(const char* name, float value);
void randomSensorData();
