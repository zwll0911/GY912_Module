#include "driver/twai.h"
#include "esp_task_wdt.h" // #5: Task Watchdog
#include <Adafruit_BMP3XX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <ICM_20948.h>
#include <Preferences.h> // #9: NVS Storage
#include <SPI.h>


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
#define WDT_TIMEOUT_SEC 5 // #5: Watchdog timeout (seconds)

// --- OBJECTS & GLOBALS ---
ICM_20948_SPI myICM;
Adafruit_BMP3XX bmp;
Preferences prefs; // #9: NVS preferences

// Create two separate LED instances
Adafruit_NeoPixel extLED(1, PIN_LED_EXT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel obLED(1, PIN_LED_OB, NEO_GRB + NEO_KHZ800);

// Shared variables (protected by mutex)
float robotYaw = 0;
float robotPitch = 0;
float robotRoll = 0;
SemaphoreHandle_t dataMutex;

// #9: NVS-stored sea-level pressure (default ISA)
float seaLevelPressure = 1013.25;

// #7: Health telemetry (written by respective tasks, read by sensor for CSV)
volatile uint32_t canTxErrors = 0;

// --- TASK PROTOTYPES ---
void taskSensor(void *pv);
void taskCAN(void *pv);
void taskLED(void *pv);

void setup() {
  Serial.begin(115200);
  pinMode(PIN_CS_ACCEL, OUTPUT);
  digitalWrite(PIN_CS_ACCEL, HIGH);
  pinMode(PIN_CS_BARO, OUTPUT);
  digitalWrite(PIN_CS_BARO, HIGH);

  // Initialize LEDs
  extLED.begin();
  extLED.setBrightness(40); // slightly dimmer for rainbow
  obLED.begin();
  obLED.setBrightness(50);

  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, -1);
  dataMutex = xSemaphoreCreateMutex();

  // --- 0. Load NVS Settings (#9) ---
  prefs.begin("nav", true); // read-only
  seaLevelPressure = prefs.getFloat("slp", 1013.25);
  prefs.end();
  Serial.print("Sea Level Pressure: ");
  Serial.println(seaLevelPressure);

  // --- 1. Initialize Sensor (DMP) ---
  myICM.begin(PIN_CS_ACCEL, SPI, 3000000);

  bool success = true;

  if (myICM.initializeDMP() != ICM_20948_Stat_Ok) {
    Serial.println("DMP Fail");
    obLED.setPixelColor(0, obLED.Color(255, 0, 0));
    obLED.show(); // Red on fail
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

  // --- 3. Initialize CAN Bus ---
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)PIN_CAN_TX, (gpio_num_t)PIN_CAN_RX, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    twai_start();
    Serial.println("CAN Active");
  }

  // --- 4. Start Tasks ---
  if (success) {
    xTaskCreatePinnedToCore(taskSensor, "IMU", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(taskCAN, "CAN", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(taskLED, "LED", 2048, NULL, 1, NULL, 0);
    Serial.println("System Active");
  }
}

void loop() { vTaskDelete(NULL); }

// --- TASK 1: SENSOR DATA (Core 1, 200Hz) ---
void taskSensor(void *pv) {
  // #5: Register this task with the watchdog
  esp_task_wdt_add(NULL);

  while (1) {
    // #5: Feed the watchdog
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

        // Euler conversion (done outside mutex, uses local vars)
        float localRoll =
            atan2(2.0 * (q0 * q1 + q2 * q3), 1.0 - 2.0 * (q1 * q1 + q2 * q2)) *
            180.0 / PI;
        float localPitch = asin(2.0 * (q0 * q2 - q3 * q1)) * 180.0 / PI;
        float localYaw =
            atan2(2.0 * (q0 * q3 + q1 * q2), 1.0 - 2.0 * (q2 * q2 + q3 * q3)) *
            180.0 / PI;

        // #6: Read barometer OUTSIDE mutex (slow SPI operation)
        float temp = 0, alt = 0, press = 0;
        if (bmp.performReading()) {
          temp = bmp.temperature;
          press = bmp.pressure / 100.0;
          alt = bmp.readAltitude(seaLevelPressure); // #9: Uses NVS value
        }

        // #1: Mutex only protects the 3 shared floats (minimal lock time)
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
          robotYaw = localYaw;
          robotPitch = localPitch;
          robotRoll = localRoll;
          xSemaphoreGive(dataMutex);
        }

        // #1: Serial output OUTSIDE mutex (no longer blocks CAN task)
        // #7: CSV format: 15 fields (added freeHeap + canTxErrors)
        Serial.print("0.0,0.0,0.0,");
        Serial.print(localPitch);
        Serial.print(",");
        Serial.print(localRoll);
        Serial.print(",");
        Serial.print(localYaw);
        Serial.print(",");
        Serial.print(localYaw);
        Serial.print(",");
        Serial.print("0.0,0.0,0.0,");
        Serial.print(temp);
        Serial.print(",");
        Serial.print(press);
        Serial.print(",");
        Serial.print(alt);
        Serial.print(",");
        Serial.print(esp_get_free_heap_size());
        Serial.print(",");
        Serial.println(canTxErrors);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// --- TASK 2: CAN BUS (Core 0, 50Hz) ---
void taskCAN(void *pv) {
  while (1) {
    // #2: Check CAN bus health and recover from bus-off
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
      canTxErrors = status.tx_error_counter; // #7: Report to telemetry

      if (status.state == TWAI_STATE_BUS_OFF) {
        // Bus-off recovery: stop, recover, restart
        twai_initiate_recovery();
        vTaskDelay(pdMS_TO_TICKS(100));
        twai_start();
        Serial.println("CAN: Bus-off recovery");
        vTaskDelay(pdMS_TO_TICKS(20));
        continue;
      }
    }

    twai_message_t message;
    message.identifier = 0x101;
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
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// --- TASK 3: HYBRID LED CONTROL (Rainbow + Blink) ---
void taskLED(void *pv) {
  uint16_t rainbowHue = 0;
  uint16_t blinkTimer = 0;
  bool blinkState = false;

  // Define Colors
  uint32_t purple = obLED.Color(60, 0, 150);
  uint32_t off = obLED.Color(0, 0, 0);

  while (1) {
    // 1. RAINBOW EFFECT (Pin 1) - Updates every loop (fast)
    // The hue goes from 0 to 65535 to circle the color wheel
    extLED.setPixelColor(0, extLED.ColorHSV(rainbowHue));
    extLED.show();

    // Increment hue for next loop (speed determined by increment size)
    rainbowHue += 250;

    // 2. PURPLE BLINK (Pin 48) - Updates every ~500ms
    blinkTimer++;
    if (blinkTimer >= 15) { // Adjust this for speed
      blinkState = !blinkState;
      if (blinkState) {
        obLED.setPixelColor(0, purple);
      } else {
        obLED.setPixelColor(0, off);
      }
      obLED.show();
      blinkTimer = 0; // Reset timer
    }

    // Loop runs at ~50Hz (20ms)
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}