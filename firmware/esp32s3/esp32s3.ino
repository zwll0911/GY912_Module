#include <Arduino.h>
#include <SPI.h>
#include <ICM_20948.h>       
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP3XX.h> 
#include <Adafruit_NeoPixel.h>
#include <MadgwickAHRS.h> 
#include "driver/twai.h"

// --- HARDWARE ---
#define PIN_SPI_SCK    12
#define PIN_SPI_MISO   13
#define PIN_SPI_MOSI   11
#define PIN_CS_ACCEL   10
#define PIN_CS_BARO    9
#define PIN_CAN_TX     5
#define PIN_CAN_RX     6
#define PIN_LED_ONBOARD 48  
#define PIN_LED_EXTERNAL 1  

// --- OBJECTS ---
ICM_20948_SPI myICM; 
Adafruit_BMP3XX bmp; 
Madgwick filter; 

Adafruit_NeoPixel onboardLED(1, PIN_LED_ONBOARD, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel externalLED(1, PIN_LED_EXTERNAL, NEO_GRB + NEO_KHZ800);

// --- VARIABLES ---
float seaLevelPressure = 1013.25;

// Calibration Storage
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;
// YOUR MAG BIAS (From previous step)
float magBiasX = -0.08; 
float magBiasY = -18.53;
float magBiasZ = -11.10;

// --- TASKS ---
TaskHandle_t TaskHandle_Sensors;
TaskHandle_t TaskHandle_LEDs;

// --- HELPERS ---
void setOnboardColor(uint8_t r, uint8_t g, uint8_t b) {
  onboardLED.setPixelColor(0, onboardLED.Color(r, g, b)); onboardLED.show();
}
void setExternalColor(uint8_t r, uint8_t g, uint8_t b) {
  externalLED.setPixelColor(0, externalLED.Color(r, g, b)); externalLED.show();
}

// ================================================================
// GYRO CALIBRATION ROUTINE (10 SECONDS)
// ================================================================
void calibrateGyro() {
  Serial.println("Calibrating Gyro... KEEP STILL for 10 Seconds!");
  setExternalColor(255, 100, 0); // Orange Warning
  
  float xSum = 0, ySum = 0, zSum = 0;
  
  // INCREASED SAMPLES: 1000 samples @ ~100Hz = ~10 Seconds
  int samples = 1000; 

  for (int i = 0; i < samples; i++) {
    // Wait for the sensor to actually have new data
    while (!myICM.dataReady()) { delay(1); } 
    
    myICM.getAGMT();
    
    xSum += myICM.gyrX();
    ySum += myICM.gyrY();
    zSum += myICM.gyrZ();
    
    // Blink the LED every 100 samples (every 1 second) so you know it's counting
    if (i % 100 == 0) {
        setExternalColor(0, 0, 0);    // OFF
        delay(50); 
        setExternalColor(255, 100, 0); // ORANGE
        Serial.print("."); // Print a dot to show progress
    }
  }
  Serial.println(" DONE!");

  // Calculate the average offset
  gyroBiasX = xSum / samples;
  gyroBiasY = ySum / samples;
  gyroBiasZ = zSum / samples;

  Serial.print("New Gyro Bias -> X:"); Serial.print(gyroBiasX);
  Serial.print(" Y:"); Serial.print(gyroBiasY);
  Serial.print(" Z:"); Serial.println(gyroBiasZ);
  
  // Flash Green 3 times to confirm success
  for(int i=0; i<3; i++) {
    setExternalColor(0, 255, 0); delay(200); 
    setExternalColor(0, 0, 0); delay(200);
  }
}

// ================================================================
// TASK 1: SENSOR FUSION ENGINE (Core 1)
// ================================================================
void Task_Sensors(void *pvParameters) {
  (void) pvParameters;
  const TickType_t xFrequency = pdMS_TO_TICKS(10); // 100Hz
  TickType_t xLastWakeTime = xTaskGetTickCount();

  filter.begin(100); 

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    if (myICM.dataReady()) {
      myICM.getAGMT(); 
      
      // 1. GET ACCEL (g)
      float ax = myICM.accX() / 1000.0;
      float ay = myICM.accY() / 1000.0;
      float az = myICM.accZ() / 1000.0;

      // 2. GET GYRO (deg/s) - SUBTRACT BIAS!
      float gx = myICM.gyrX() - gyroBiasX;
      float gy = myICM.gyrY() - gyroBiasY;
      float gz = myICM.gyrZ() - gyroBiasZ;

      // 3. GET MAG (uT) - SUBTRACT BIAS!
      float mx = myICM.magX() - magBiasX; 
      float my = myICM.magY() - magBiasY;
      float mz = myICM.magZ() - magBiasZ;

      // 4. FUSION
      filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);

      float pitch = filter.getPitch();
      float roll  = filter.getRoll();
      float yaw   = filter.getYaw();

      // 5. READ BARO & SEND
      if (bmp.performReading()) {
        float alt = bmp.readAltitude(seaLevelPressure);

        // --- CAN TRANSMISSION ---
        // Frame 1: ACCEL (ID 0x100)
        twai_message_t msg1;
        msg1.identifier = 0x100; msg1.extd = 0; msg1.data_length_code = 6;
        int16_t c_ax = (int16_t)(ax * 1000); 
        int16_t c_ay = (int16_t)(ay * 1000);
        int16_t c_az = (int16_t)(az * 1000);
        msg1.data[0] = highByte(c_ax); msg1.data[1] = lowByte(c_ax);
        msg1.data[2] = highByte(c_ay); msg1.data[3] = lowByte(c_ay);
        msg1.data[4] = highByte(c_az); msg1.data[5] = lowByte(c_az);
        twai_transmit(&msg1, 0);

        // Frame 2: ORIENTATION (ID 0x101)
        twai_message_t msg2;
        msg2.identifier = 0x101; msg2.extd = 0; msg2.data_length_code = 6;
        int16_t c_p = (int16_t)(pitch * 100);
        int16_t c_r = (int16_t)(roll * 100);
        int16_t c_y = (int16_t)(yaw * 100);
        msg2.data[0] = highByte(c_p); msg2.data[1] = lowByte(c_p);
        msg2.data[2] = highByte(c_r); msg2.data[3] = lowByte(c_r);
        msg2.data[4] = highByte(c_y); msg2.data[5] = lowByte(c_y);
        twai_transmit(&msg2, 0);

        // Frame 3: ALTITUDE (ID 0x102)
        twai_message_t msg3;
        msg3.identifier = 0x102; msg3.extd = 0; msg3.data_length_code = 2;
        int16_t c_alt = (int16_t)(alt * 100);
        msg3.data[0] = highByte(c_alt); msg3.data[1] = lowByte(c_alt);
        twai_transmit(&msg3, 0);

        // --- SERIAL DASHBOARD ---
        Serial.print(ax); Serial.print(","); Serial.print(ay); Serial.print(","); Serial.print(az); Serial.print(",");
        Serial.print(pitch); Serial.print(","); Serial.print(roll); Serial.print(","); Serial.print(yaw); Serial.print(",");
        Serial.print(mx); Serial.print(","); Serial.print(my); Serial.print(","); Serial.print(mz); Serial.print(",");
        Serial.print(bmp.temperature); Serial.print(","); Serial.print(bmp.pressure/100.0); Serial.print(","); Serial.println(alt);

        setOnboardColor(50, 0, 100); delay(1); setOnboardColor(0, 0, 0); 
      }
    }
  }
}

// ================================================================
// TASK 2: LED ANIMATION (Core 0)
// ================================================================
void Task_LEDs(void *pvParameters) {
  (void) pvParameters;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(20); 

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    // Purple Breathing
    float val = (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;
    setExternalColor((int)(val * 0.8), 0, (int)val); 
  }
}

// ================================================================
// SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Pins
  pinMode(PIN_CS_ACCEL, OUTPUT); pinMode(PIN_CS_BARO, OUTPUT);
  digitalWrite(PIN_CS_ACCEL, HIGH); digitalWrite(PIN_CS_BARO, HIGH);

  // LEDs
  onboardLED.begin(); onboardLED.setBrightness(100);
  externalLED.begin(); externalLED.setBrightness(200);
  setOnboardColor(255, 165, 0); setExternalColor(255, 165, 0);

  Serial.println("\n--- NAV MODULE (STABLE YAW) ---");

  // Sensors
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, -1);
  myICM.begin(PIN_CS_ACCEL, SPI); 
  if (myICM.status != ICM_20948_Stat_Ok) while(1);

  ICM_20948_fss_t myFSS; myFSS.a = gpm4; myFSS.g = dps500;
  myICM.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myFSS);

  if (!bmp.begin_SPI(PIN_CS_BARO, &SPI)) while(1);
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  // CAN
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)PIN_CAN_TX, (gpio_num_t)PIN_CAN_RX, TWAI_MODE_NO_ACK);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  twai_driver_install(&g_config, &t_config, &f_config);
  twai_start();

  // --- RUN CALIBRATION ---
  calibrateGyro(); 

  // Create Tasks
  xTaskCreatePinnedToCore(Task_Sensors, "Sensors", 4096, NULL, 2, &TaskHandle_Sensors, 1);
  xTaskCreatePinnedToCore(Task_LEDs, "LEDs", 2048, NULL, 1, &TaskHandle_LEDs, 0);
}

void loop() { vTaskDelete(NULL); }