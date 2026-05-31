
 #define BLYNK_PRINT Serial

// 🔹 Blynk Credentials (Keep these secure)
#define BLYNK_TEMPLATE_ID "TMPL3V6xXVcL1"
#define BLYNK_TEMPLATE_NAME "Fall detection System using iot"
#define BLYNK_AUTH_TOKEN "KQ0g-28Afln2O4Yr21EdAtAQAHiW2pui"

// 🔹 ESP32 Libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>

// 🔹 WiFi Credentials
char ssid[] = "smart";
char pass[] = "123456789";

// 🔹 Pin Definitions for LED and Buzzer
#define LED_PIN 2      // GPIO 2 (Often the onboard LED)
#define BUZZER_PIN 15  // GPIO 15 for the Buzzer

// MPU6050 Address
const int MPU_addr = 0x68;

// Sensor variables
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;
float ax, ay, az, gx, gy, gz;

// Fall detection variables
boolean fall = false;
boolean trigger1 = false;
boolean trigger2 = false;
boolean trigger3 = false;
byte trigger1count = 0;
byte trigger2count = 0;
byte trigger3count = 0;
int angleChange = 0;

void setup() {
  Serial.begin(115200);

  // Initialize LED and Buzzer pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);    // Ensure they start OFF
  digitalWrite(BUZZER_PIN, LOW);

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // MPU6050 setup
  Wire.begin(); // ESP32 Default I2C: SDA = 21, SCL = 22
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // Wake up MPU6050
  Wire.endTransmission(true);
  
  Serial.println("MPU6050 Ready");
}

void loop() {
  Blynk.run();
  mpu_read();
  
  // Convert raw values
  ax = AcX / 16384.0;
  ay = AcY / 16384.0;
  az = AcZ / 16384.0;
  gx = GyX / 131.07;
  gy = GyY / 131.07;
  gz = GyZ / 131.07;
  
  // Amplitude calculation
  float Raw_Amp = sqrt(ax * ax + ay * ay + az * az);
  int Amp = Raw_Amp * 10;
  Serial.println(Amp);
  
  // Trigger 1 (Free fall - low amplitude)
  if (Amp <= 2 && !trigger2) {
    trigger1 = true;
    Serial.println("TRIGGER 1");
  }
  
  // Trigger 2 (Impact - high amplitude)
  if (trigger1) {
    trigger1count++;
    if (Amp >= 12) {
      trigger2 = true;
      trigger1 = false;
      trigger1count = 0;
      Serial.println("TRIGGER 2");
    }
  }
  
  // Trigger 3 (Orientation change)
  if (trigger2) {
    trigger2count++;
    angleChange = sqrt(gx * gx + gy * gy + gz * gz);
    if (angleChange >= 30 && angleChange <= 400) {
      trigger3 = true;
      trigger2 = false;
      trigger2count = 0;
      Serial.println("TRIGGER 3");
    }
  }
  
  // Fall detection verification (No movement after fall)
  if (trigger3) {
    trigger3count++;
    if (trigger3count >= 10) {
      angleChange = sqrt(gx * gx + gy * gy + gz * gz);
      if (angleChange <= 20) {
        fall = true;
        trigger3 = false;
        trigger3count = 0;
      } else {
        trigger3 = false;
        trigger3count = 0;
      }
    }
  }
  
  // ✅ FALL DETECTED
  if (fall) {
    Serial.println("FALL DETECTED");
    
    // Activate Local Alarms
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    // Send to Blynk
    Blynk.logEvent("fall_alert", "Fall Detected! Take action immediately.");
    Blynk.virtualWrite(V0, 1);
    
    // Wait for 2 seconds while alarm rings
    delay(2000);
    
    // Deactivate Local Alarms & Reset Virtual Pin
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Blynk.virtualWrite(V0, 0);
    
    fall = false;
  }
  
  // Reset triggers to prevent false positives
  if (trigger2count >= 6) {
    trigger2 = false;
    trigger2count = 0;
  }
  if (trigger1count >= 6) {
    trigger1 = false;
    trigger1count = 0;
  }
  
  delay(100);
}

// MPU Read Function
void mpu_read() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); // Starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)MPU_addr, (uint8_t)14);
  
  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read(); // skip temp
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();
}