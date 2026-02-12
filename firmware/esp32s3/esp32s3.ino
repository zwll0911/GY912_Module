#include "driver/twai.h"
#include "esp_task_wdt.h"
#include <Adafruit_BMP3XX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <ArduinoOTA.h> // #3: OTA
#include <ICM_20948.h>
#include <Preferences.h>
#include <SPI.h>
#include <WiFi.h>    // #3: OTA
#include <WiFiUdp.h> // UDP low-latency streaming

// --- PINS ---
#define PIN_SPI_SCK 12
#define PIN_SPI_MISO 13
#define PIN_SPI_MOSI 11
#define PIN_CS_ACCEL 10
#define PIN_CS_BARO 9
#define PIN_CAN_TX 5
#define PIN_CAN_RX 6
#define PIN_LED_EXT 1 // Rainbow LED
#define PIN_LED_OB 48 // Blinking Purple LED

// --- WATCHDOG ---
#define WDT_TIMEOUT_SEC 5

// --- #5: ALTITUDE SMOOTHING ---
#define ALT_AVG_SIZE 10

// --- #2: CAN RECEIVE COMMAND IDs ---
#define CAN_TX_ID_DEFAULT 0x101 // Outgoing: orientation data
#define CAN_RX_CMD_ID 0x200     // Incoming: commands
#define CAN_TX_HEARTBEAT 0x102  // Outgoing: heartbeat reply
// Command bytes for 0x200:
#define CMD_TARE 0x01
#define CMD_SET_SLP 0x02
#define CMD_HEARTBEAT 0x03

// --- OBJECTS & GLOBALS ---
ICM_20948_SPI myICM;
Adafruit_BMP3XX bmp;
Preferences prefs;

Adafruit_NeoPixel extLED(1, PIN_LED_EXT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel obLED(1, PIN_LED_OB, NEO_GRB + NEO_KHZ800);

// Shared variables (protected by mutex)
float robotYaw = 0;
float robotPitch = 0;
float robotRoll = 0;
float yawTareOffset = 0; // #2: Remote tare offset
SemaphoreHandle_t dataMutex;

// #9/#4: NVS-stored settings
float seaLevelPressure = 1013.25;
uint32_t canTxId = CAN_TX_ID_DEFAULT;
uint16_t canTxIntervalMs = 20; // 50Hz default

// Health telemetry
volatile uint32_t canTxErrors = 0;

// #1: Drift tracking
float yawDriftRate = 0;

// #5: Altitude moving average buffer
float altBuffer[ALT_AVG_SIZE];
uint8_t altBufIdx = 0;
bool altBufFull = false;

// #3: OTA state
bool otaEnabled = false;
bool otaInitialized = false;

// UDP streaming
unsigned long lastUdpSendMs = 0;
WiFiUDP udp;
const uint16_t UDP_PORT = 4210;
const IPAddress UDP_BROADCAST(192, 168, 4, 255);

// --- TASK PROTOTYPES ---
void taskSensor(void *pv);
void taskCAN(void *pv);
void taskLED(void *pv);

// --- HELPER: #5 Altitude Moving Average ---
float addAltSample(float sample) {
  altBuffer[altBufIdx] = sample;
  altBufIdx = (altBufIdx + 1) % ALT_AVG_SIZE;
  if (!altBufFull && altBufIdx == 0)
    altBufFull = true;

  uint8_t count = altBufFull ? ALT_AVG_SIZE : altBufIdx;
  if (count == 0)
    return sample;
  float sum = 0;
  for (uint8_t i = 0; i < count; i++)
    sum += altBuffer[i];
  return sum / count;
}

// --- HELPER: #3 OTA + UDP Setup ---
void setupOTA() {
  if (otaInitialized)
    return;

  WiFi.mode(WIFI_AP);
  WiFi.softAP("NAV_MODULE_OTA", "navmodule");
  Serial.print("OTA AP IP: ");
  Serial.println(WiFi.softAPIP());

  ArduinoOTA.setHostname("nav-module");
  ArduinoOTA.onStart([]() { Serial.println("OTA: Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("OTA: Done, rebooting..."); });
  ArduinoOTA.onError(
      [](ota_error_t error) { Serial.printf("OTA Error [%u]\n", error); });
  ArduinoOTA.begin();

  // Start UDP for low-latency sensor streaming
  udp.begin(UDP_PORT);
  Serial.printf("UDP: Broadcasting on port %d\n", UDP_PORT);

  otaInitialized = true;
  Serial.println("OTA: Ready (connect to NAV_MODULE_OTA)");
}

void stopOTA() {
  if (!otaInitialized)
    return;
  udp.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  otaInitialized = false;
  Serial.println("OTA: Disabled");
}

// --- HELPER: #2 Save SLP to NVS ---
void saveSLP(float newSLP) {
  seaLevelPressure = newSLP;
  prefs.begin("nav", false);
  prefs.putFloat("slp", seaLevelPressure);
  prefs.end();
  Serial.print("SLP saved: ");
  Serial.println(seaLevelPressure);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_CS_ACCEL, OUTPUT);
  digitalWrite(PIN_CS_ACCEL, HIGH);
  pinMode(PIN_CS_BARO, OUTPUT);
  digitalWrite(PIN_CS_BARO, HIGH);

  // Initialize LEDs
  extLED.begin();
  extLED.setBrightness(40);
  obLED.begin();
  obLED.setBrightness(50);

  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, -1);
  dataMutex = xSemaphoreCreateMutex();

  // --- 0. Load NVS Settings ---
  prefs.begin("nav", true);
  seaLevelPressure = prefs.getFloat("slp", 1013.25);
  canTxId = prefs.getUInt("canId", CAN_TX_ID_DEFAULT);
  canTxIntervalMs = prefs.getUShort("canMs", 20);
  otaEnabled = prefs.getBool("ota", false);
  prefs.end();
  Serial.printf("Config: SLP=%.2f, CAN_ID=0x%03X, CAN_ms=%d, OTA=%s\n",
                seaLevelPressure, canTxId, canTxIntervalMs,
                otaEnabled ? "ON" : "OFF");

  // --- 1. Initialize Sensor (DMP) ---
  myICM.begin(PIN_CS_ACCEL, SPI, 3000000);

  bool success = true;
  if (myICM.initializeDMP() != ICM_20948_Stat_Ok) {
    Serial.println("DMP Fail");
    obLED.setPixelColor(0, obLED.Color(255, 0, 0));
    obLED.show();
    while (1)
      ;
  }

  success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_GAME_ROTATION_VECTOR,
                                    true) == ICM_20948_Stat_Ok);
  success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Quat6, 0) == ICM_20948_Stat_Ok);
  success &= (myICM.enableFIFO() == ICM_20948_Stat_Ok);
  success &= (myICM.enableDMP() == ICM_20948_Stat_Ok);
  myICM.resetDMP();
  myICM.resetFIFO();

  // --- 2. Initialize Barometer ---
  if (!bmp.begin_SPI(PIN_CS_BARO, &SPI)) {
    Serial.println("BMP388 Fail");
  } else {
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
    bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  }

  // Initialize altitude buffer
  for (int i = 0; i < ALT_AVG_SIZE; i++)
    altBuffer[i] = 0;

  // --- 3. Initialize CAN Bus ---
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)PIN_CAN_TX, (gpio_num_t)PIN_CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    twai_start();
    Serial.println("CAN Active");
  }

  // --- 4. OTA (if enabled in NVS) ---
  if (otaEnabled) {
    setupOTA();
  }

  // --- 5. Start Tasks ---
  if (success) {
    xTaskCreatePinnedToCore(taskSensor, "IMU", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(taskCAN, "CAN", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(taskLED, "LED", 2048, NULL, 1, NULL, 0);
    Serial.println("System Active");
  }
}

void loop() {
  // Handle serial commands for OTA toggle
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "OTA_ON") {
      otaEnabled = true;
      prefs.begin("nav", false);
      prefs.putBool("ota", true);
      prefs.end();
      setupOTA();
    } else if (cmd == "OTA_OFF") {
      otaEnabled = false;
      prefs.begin("nav", false);
      prefs.putBool("ota", false);
      prefs.end();
      stopOTA();
    }
  }
  if (otaInitialized) {
    ArduinoOTA.handle();
    vTaskDelay(pdMS_TO_TICKS(50));
  } else {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// --- TASK 1: SENSOR DATA (Core 1, 200Hz) ---
void taskSensor(void *pv) {
  esp_task_wdt_add(NULL);

  // #1: Drift tracking variables
  float prevYaw = 0;
  unsigned long prevDriftTime = millis();
  float driftAccum = 0;
  uint16_t driftSamples = 0;

  while (1) {
    esp_task_wdt_reset();

    icm_20948_DMP_data_t data;
    myICM.readDMPdataFromFIFO(&data);

    if ((myICM.status == ICM_20948_Stat_Ok) ||
        (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
      if ((data.header & DMP_header_bitmap_Quat6) > 0) {

        double q1 = ((double)data.Quat6.Data.Q1) / 1073741824.0;
        double q2 = ((double)data.Quat6.Data.Q2) / 1073741824.0;
        double q3 = ((double)data.Quat6.Data.Q3) / 1073741824.0;
        double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));

        float localRoll =
            atan2(2.0 * (q0 * q1 + q2 * q3), 1.0 - 2.0 * (q1 * q1 + q2 * q2)) *
            180.0 / PI;
        float localPitch = asin(2.0 * (q0 * q2 - q3 * q1)) * 180.0 / PI;
        float localYaw =
            atan2(2.0 * (q0 * q3 + q1 * q2), 1.0 - 2.0 * (q2 * q2 + q3 * q3)) *
            180.0 / PI;

        // #1: Calculate yaw drift rate (°/s, EMA)
        unsigned long now = millis();
        float dt = (now - prevDriftTime) / 1000.0;
        if (dt > 0.001) {
          float instantDrift = fabs(localYaw - prevYaw) / dt;
          // Handle wraparound at ±180
          if (instantDrift > 180.0 / dt)
            instantDrift = fabs(360.0 - instantDrift * dt) / dt;
          // Exponential moving average (alpha = 0.05 for smooth)
          yawDriftRate = yawDriftRate * 0.95 + instantDrift * 0.05;
          prevYaw = localYaw;
          prevDriftTime = now;
        }

        // Read barometer (outside mutex)
        float temp = 0, press = 0, rawAlt = 0, smoothAlt = 0;
        if (bmp.performReading()) {
          temp = bmp.temperature;
          press = bmp.pressure / 100.0;
          rawAlt = bmp.readAltitude(seaLevelPressure);
          smoothAlt = addAltSample(rawAlt); // #5: Moving average
        }

        // Mutex: only write shared floats
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
          robotYaw = localYaw;
          robotPitch = localPitch;
          robotRoll = localRoll;
          xSemaphoreGive(dataMutex);
        }

        // Send to active channel (throttled to 50Hz)
        if (otaInitialized) {
          unsigned long now_udp = millis();
          if (now_udp - lastUdpSendMs >= 20) {
            // Build CSV only when sending
            char csvLine[200];
            snprintf(
                csvLine, sizeof(csvLine),
                "0.0,0.0,0.0,%.2f,%.2f,%.2f,%.2f,0.0,0.0,0.0,%.1f,%.1f,%.1f,%"
                "lu,%lu,%.4f",
                localPitch, localRoll, localYaw, localYaw, temp, press,
                smoothAlt, (unsigned long)esp_get_free_heap_size(),
                (unsigned long)canTxErrors, yawDriftRate);
            udp.beginPacket(UDP_BROADCAST, UDP_PORT);
            udp.print(csvLine);
            udp.endPacket();
            lastUdpSendMs = now_udp;
          }
        } else if (Serial) {
          char csvLine[200];
          snprintf(
              csvLine, sizeof(csvLine),
              "0.0,0.0,0.0,%.2f,%.2f,%.2f,%.2f,0.0,0.0,0.0,%.1f,%.1f,%.1f,%"
              "lu,%lu,%.4f",
              localPitch, localRoll, localYaw, localYaw, temp, press, smoothAlt,
              (unsigned long)esp_get_free_heap_size(),
              (unsigned long)canTxErrors, yawDriftRate);
          Serial.println(csvLine);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// --- TASK 2: CAN BUS (Core 0, configurable Hz) ---
void taskCAN(void *pv) {
  while (1) {
    // --- Health check & bus-off recovery ---
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
      canTxErrors = status.tx_error_counter;

      if (status.state == TWAI_STATE_BUS_OFF) {
        twai_initiate_recovery();
        vTaskDelay(pdMS_TO_TICKS(100));
        twai_start();
        Serial.println("CAN: Bus-off recovery");
        vTaskDelay(pdMS_TO_TICKS(canTxIntervalMs));
        continue;
      }
    }

    // --- #2: Receive incoming commands ---
    twai_message_t rxMsg;
    while (twai_receive(&rxMsg, 0) == ESP_OK) { // Non-blocking poll
      if (rxMsg.identifier == CAN_RX_CMD_ID && rxMsg.data_length_code >= 1) {
        switch (rxMsg.data[0]) {
        case CMD_TARE: {
          // Remote tare: set current yaw as zero reference
          if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5))) {
            yawTareOffset = robotYaw;
            xSemaphoreGive(dataMutex);
          }
          Serial.println("CAN: Remote TARE");
          break;
        }
        case CMD_SET_SLP: {
          // Set sea-level pressure: bytes[1-4] = float (little-endian)
          if (rxMsg.data_length_code >= 5) {
            float newSLP;
            memcpy(&newSLP, &rxMsg.data[1], sizeof(float));
            if (newSLP > 800.0 && newSLP < 1200.0) { // Sanity check
              saveSLP(newSLP);
            }
          }
          break;
        }
        case CMD_HEARTBEAT: {
          // Reply with heartbeat on 0x102
          twai_message_t hbReply;
          hbReply.identifier = CAN_TX_HEARTBEAT;
          hbReply.data_length_code = 2;
          hbReply.data[0] = 0xAC; // ACK
          hbReply.data[1] =
              (uint8_t)(esp_get_free_heap_size() / 1024); // Heap KB
          twai_transmit(&hbReply, pdMS_TO_TICKS(5));
          break;
        }
        }
      }
    }

    // --- Transmit orientation data ---
    twai_message_t message;
    message.identifier = canTxId; // #4: Configurable
    message.data_length_code = 6;

    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      int16_t y = (int16_t)(robotYaw * 100);
      int16_t p = (int16_t)(robotPitch * 100);
      int16_t r = (int16_t)(robotRoll * 100);
      xSemaphoreGive(dataMutex);

      message.data[0] = (y >> 8) & 0xFF;
      message.data[1] = y & 0xFF;
      message.data[2] = (p >> 8) & 0xFF;
      message.data[3] = p & 0xFF;
      message.data[4] = (r >> 8) & 0xFF;
      message.data[5] = r & 0xFF;

      twai_transmit(&message, pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(canTxIntervalMs)); // #4: Configurable
  }
}

// --- TASK 3: STATUS LED CONTROL ---
void taskLED(void *pv) {
  uint16_t blinkTimer = 0;
  bool blinkState = false;
  int16_t breathVal = 10; // Use signed to avoid underflow
  int16_t breathDir = 5;

  uint32_t purple = obLED.Color(60, 0, 150);
  uint32_t off = obLED.Color(0, 0, 0);

  while (1) {
    // External LED: OTA/WiFi status indicator
    if (otaInitialized) {
      // Pulsing blue = WiFi/OTA active
      breathVal += breathDir;
      if (breathVal >= 200) {
        breathVal = 200;
        breathDir = -5;
      }
      if (breathVal <= 10) {
        breathVal = 10;
        breathDir = 5;
      }
      extLED.setPixelColor(0, extLED.Color(0, 0, (uint8_t)breathVal));
    } else {
      // Solid green = normal operation
      extLED.setPixelColor(0, extLED.Color(0, 80, 0));
    }
    extLED.show();

    // Onboard LED: heartbeat blink (purple)
    blinkTimer++;
    if (blinkTimer >= 15) {
      blinkState = !blinkState;
      obLED.setPixelColor(0, blinkState ? purple : off);
      obLED.show();
      blinkTimer = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz is fine for status LEDs
  }
}