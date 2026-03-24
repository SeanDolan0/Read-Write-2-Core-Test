#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "SdFunction.h"

// ─────────────────────────────────────────────
//  Config
// ─────────────────────────────────────────────
#define SD_CS_PIN        5
#define MUTEX_TIMEOUT_MS 5000

const uint32_t WRITE_INTERVAL_MS = 1000;
const char*    CSV_FILE_PATH     = "/sensor_log.csv";
uint32_t       lastWriteTime     = 0;
bool           sdReady           = false;

// ─────────────────────────────────────────────
//  Ring buffer  (single source of truth)
// ─────────────────────────────────────────────
SensorRecord     recordBuffer[MAX_RECORDS];
volatile uint16_t recordHead = 0;
volatile uint16_t sdTail     = 0;
volatile uint16_t rbTail     = 0;

// ─────────────────────────────────────────────
//  ASCII flush buffer (SD writes)
// ─────────────────────────────────────────────
static char   asciiBuffer[512];
static size_t asciiBufferLen = 0;

// ─────────────────────────────────────────────
//  SD initialisation
// ─────────────────────────────────────────────
bool initSDCard() {
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Card Mount Failed");
        return false;
    }

    uint64_t cardSize = SD.cardSize() / (1000ULL * 1000 * 1000);
    Serial.printf("SD Card Size: %lluGB\n", cardSize);

    if (!SD.exists(CSV_FILE_PATH)) {
        File f = SD.open(CSV_FILE_PATH, FILE_WRITE);
        if (!f) {
            Serial.println("Failed to create CSV file");
            return false;
        }
        f.print("timestamp_ms");
        for (int i = 0; i < MAX_SENSORS; i++) {
            f.print(",");
            f.print(SENSOR_NAMES[i]);
        }
        f.println();
        f.close();
    }
    return true;
}

// ─────────────────────────────────────────────
//  ASCII buffer → SD
// ─────────────────────────────────────────────
bool LogWriteBuffer() {
    if (asciiBufferLen == 0) return true;

    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        Serial.println("LogWriteBuffer: mutex timeout");
        return false;
    }

    File f = SD.open(CSV_FILE_PATH, FILE_APPEND);
    if (!f) {
        Serial.println("LogWriteBuffer: failed to open file");
        xSemaphoreGive(logMutex);
        return false;
    }

    size_t written = f.write((const uint8_t*)asciiBuffer, asciiBufferLen);
    f.close();
    xSemaphoreGive(logMutex);

    if (written != asciiBufferLen) {
        Serial.println("LogWriteBuffer: partial write");
        return false;
    }

    asciiBufferLen = 0;
    return true;
}

// ─────────────────────────────────────────────
//  Internal: format one SensorRecord as a CSV
//  row and append to asciiBuffer.
//  Flushes to SD automatically on overflow.
// ─────────────────────────────────────────────
static void appendRecordToAsciiBuffer(const SensorRecord& rec) {
    char row[128];
    int  pos = snprintf(row, sizeof(row), "%lu", (unsigned long)rec.timestamp_ms);

    for (int i = 0; i < MAX_SENSORS; i++) {
        if (rec.sensor_mask & (1 << i)) {
            // Convert scaled int back to float for human-readable CSV
            float val = rec.values[i] / (float)SENSOR_SCALE[i];
            int w = snprintf(row + pos, sizeof(row) - pos, ",%.2f", val);
            if (w < 0 || pos + w >= (int)sizeof(row)) {
                Serial.println("appendRecord: row too long");
                return;
            }
            pos += w;
        } else {
            // Sensor not present in this record — leave cell empty
            if (pos + 1 >= (int)sizeof(row)) return;
            row[pos++] = ',';
        }
    }
    row[pos++] = '\n';

    if (asciiBufferLen + pos >= sizeof(asciiBuffer)) {
        Serial.println("ASCII buffer full, flushing to SD");
        LogWriteBuffer();
    }
    memcpy(asciiBuffer + asciiBufferLen, row, pos);
    asciiBufferLen += pos;
}

// ─────────────────────────────────────────────
//  Write one sensor reading into the ring buffer.
//  If the named sensor already has a record at
//  recordHead-1 that hasn't been sealed yet
//  (same ms bucket), we update it in place;
//  otherwise we open a new record.
//
//  "Sealed" here just means: a new timestamp
//  arrived, so the previous record is complete.
// ─────────────────────────────────────────────
void writeDataToBuffer(const char* name, float value) {
    // Find sensor slot
    int slot = -1;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (strcmp(name, SENSOR_NAMES[i]) == 0) { slot = i; break; }
    }
    if (slot == -1) {
        Serial.printf("writeDataToBuffer: unknown sensor '%s'\n", name);
        return;
    }

    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        Serial.println("writeDataToBuffer: mutex timeout");
        return;
    }

    uint32_t now      = millis();
    int16_t  scaled   = (int16_t)(value * SENSOR_SCALE[slot]);
    uint16_t prevIdx  = (recordHead - 1) & (MAX_RECORDS - 1);

    // Reuse the last record if it was created in the same millisecond
    // and that slot hasn't been written yet
    bool reuseExisting = (recordHead != sdTail) &&          // buffer not empty from SD's perspective
                         (recordHead != rbTail) &&          // and from RB's perspective
                         (recordBuffer[prevIdx].timestamp_ms == now) &&
                         !(recordBuffer[prevIdx].sensor_mask & (1 << slot));

    // Actually we want to reuse if the last written record exists at all
    // (even if tails have passed it, the slot is just overwritten):
    // Simpler rule: reuse if prevIdx record has same timestamp and slot is free
    uint16_t head     = recordHead;
    bool headIsEmpty  = (head == sdTail && head == rbTail); // truly nothing yet

    if (!headIsEmpty &&
        recordBuffer[prevIdx].timestamp_ms == now &&
        !(recordBuffer[prevIdx].sensor_mask & (1 << slot)))
    {
        // Update existing record in place
        recordBuffer[prevIdx].values[slot]      = scaled;
        recordBuffer[prevIdx].sensor_mask       |= (1 << slot);
    } else {
        // Open a new record
        // Check for overflow: if head is about to lap both tails, warn
        uint16_t nextHead = (head + 1) & (MAX_RECORDS - 1);
        if (nextHead == sdTail || nextHead == rbTail) {
            Serial.println("writeDataToBuffer: ring buffer full, oldest record overwritten");
        }

        recordBuffer[head].timestamp_ms = now;
        recordBuffer[head].sensor_mask  = (1 << slot);
        memset(recordBuffer[head].values, 0, sizeof(recordBuffer[head].values));
        recordBuffer[head].values[slot] = scaled;
        recordHead = nextHead;

        // Also format the previous (now sealed) record into the ASCII buffer
        // so SD always has complete records. Skip on the very first write.
        if (head != 0 || sdTail != 0) {
            appendRecordToAsciiBuffer(recordBuffer[prevIdx]);
        }
    }

    xSemaphoreGive(logMutex);
}

// ─────────────────────────────────────────────
//  Binary packer for RockBlock
//
//  Message layout (little-endian):
//  [1B version][1B num_sensors][2B sensor_mask][2B num_records]
//  then per record:
//    [4B timestamp_ms][2B * popcount(sensor_mask) values]
//
//  Only the sensors present in the *message-level* mask are written.
//  The message mask is the OR of all records' masks in this batch.
// ─────────────────────────────────────────────
size_t packRockBlockMessage(uint8_t* buf, size_t bufSize) {
    if (bufSize < 6) return 0;  // not even room for a header

    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        Serial.println("packRockBlockMessage: mutex timeout");
        return 0;
    }

    uint16_t head    = recordHead;
    uint16_t tail    = rbTail;
    uint16_t pending = (head - tail) & (MAX_RECORDS - 1);

    if (pending == 0) {
        xSemaphoreGive(logMutex);
        return 0;
    }

    // ── Pass 1: build the union sensor mask for this batch ──
    uint16_t msgMask    = 0;
    uint16_t recordsThatFit = 0;

    // Work out bytes-per-record for a growing mask, stop when we hit 340B
    // Header = 6 bytes; each record = 4 + 2*popcount(msgMask) bytes
    // We re-evaluate as we scan because msgMask can grow
    size_t   headerSize = 6;
    size_t   used       = headerSize;

    for (uint16_t i = 0; i < pending; i++) {
        uint16_t idx  = (tail + i) & (MAX_RECORDS - 1);
        uint16_t mask = recordBuffer[idx].sensor_mask;
        uint16_t newMsgMask = msgMask | mask;
        int      sensors    = __builtin_popcount(newMsgMask);
        size_t   recSize    = 4 + 2 * sensors;   // timestamp + values

        if (used + recSize > bufSize) break;

        msgMask = newMsgMask;
        used   += recSize;
        recordsThatFit++;
    }

    if (recordsThatFit == 0) {
        xSemaphoreGive(logMutex);
        Serial.println("packRockBlockMessage: single record too large for buffer");
        return 0;
    }

    // ── Write header ──
    size_t pos = 0;
    buf[pos++] = 0x01;                          // version
    buf[pos++] = (uint8_t)__builtin_popcount(msgMask);
    buf[pos++] = (uint8_t)(msgMask & 0xFF);
    buf[pos++] = (uint8_t)(msgMask >> 8);
    buf[pos++] = (uint8_t)(recordsThatFit & 0xFF);
    buf[pos++] = (uint8_t)(recordsThatFit >> 8);

    // ── Pass 2: write records ──
    for (uint16_t i = 0; i < recordsThatFit; i++) {
        uint16_t idx = (tail + i) & (MAX_RECORDS - 1);
        const SensorRecord& rec = recordBuffer[idx];

        // timestamp (4 bytes, little-endian)
        buf[pos++] = (rec.timestamp_ms >>  0) & 0xFF;
        buf[pos++] = (rec.timestamp_ms >>  8) & 0xFF;
        buf[pos++] = (rec.timestamp_ms >> 16) & 0xFF;
        buf[pos++] = (rec.timestamp_ms >> 24) & 0xFF;

        // values: only slots present in msgMask, in fixed slot order
        for (int s = 0; s < MAX_SENSORS; s++) {
            if (!(msgMask & (1 << s))) continue;
            int16_t v = (rec.sensor_mask & (1 << s)) ? rec.values[s] : 0;
            buf[pos++] = (uint8_t)(v & 0xFF);
            buf[pos++] = (uint8_t)((v >> 8) & 0xFF);
        }
    }

    // Advance RockBlock tail
    rbTail = (tail + recordsThatFit) & (MAX_RECORDS - 1);

    xSemaphoreGive(logMutex);

    Serial.printf("packRockBlockMessage: packed %u records, %u bytes\n",
                  recordsThatFit, (unsigned)pos);
    return pos;
}

// ─────────────────────────────────────────────
//  Test stub — random sensor data
// ─────────────────────────────────────────────
static uint32_t nextTempTime     = 0;
static uint32_t nextPressureTime = 0;
static uint32_t nextHumidityTime = 0;

void randomSensorData() {
    uint32_t now = millis();
    if (now >= nextTempTime) {
        writeDataToBuffer("temp",     random(200, 301)   / 10.0f);
        nextTempTime     = now + random(50, 500);
    }
    if (now >= nextPressureTime) {
        writeDataToBuffer("pressure", random(9000, 11001) / 10.0f);
        nextPressureTime = now + random(300, 2000);
    }
    if (now >= nextHumidityTime) {
        writeDataToBuffer("humidity", random(200, 901)   / 10.0f);
        nextHumidityTime = now + random(100, 1000);
    }
}