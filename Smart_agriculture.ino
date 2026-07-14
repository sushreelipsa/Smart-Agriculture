#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL3X7xKJ2OI"
#define BLYNK_TEMPLATE_NAME "Smart Agriculture 1"
#define BLYNK_AUTH_TOKEN "aBvcXKHL4Rp_MkfA137rhMC33f4eZK5r"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// -------- WIFI --------
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Shubham's M14";
char pass[] = "shubhamroul";

// -------- PIN DEFINITIONS --------
#define SOIL1_PIN   34
#define SOIL2_PIN   33
#define SMOKE_PIN   35
#define PIR_PIN     27
#define DHT_PIN     4
#define PUMP_PIN    26
#define BUZZER_PIN  25

#define DHTTYPE DHT11

// -------- CALIBRATION --------
#define SOIL_DRY  3800
#define SOIL_WET  1200

// -------- THRESHOLDS --------
#define FIELD1_THRESHOLD 30
#define FIELD2_THRESHOLD 40
#define FIRE_THRESHOLD 60

DHT dht(DHT_PIN, DHTTYPE);
BlynkTimer timer;

bool manualPump = false;
bool fireAlertSent = false;
unsigned long lastFireAlertTime = 0;
const unsigned long fireCooldown = 15000; // 15 sec cooldown

unsigned long lastDataReportTime = 0;
const unsigned long dataReportInterval = 60000; // 1 min

// -------- MANUAL PUMP CONTROL (V6) --------
BLYNK_WRITE(V6) {
  manualPump = param.asInt();
}

// -------- SENSOR FUNCTION --------
void sendSensorData() {
  // ===== SOIL FIELD 1 =====
  int soilRaw1 = analogRead(SOIL1_PIN);
  int soilPercent1 = constrain(map(soilRaw1, SOIL_DRY, SOIL_WET, 0, 100), 0, 100);

  // ===== SOIL FIELD 2 =====
  int soilRaw2 = analogRead(SOIL2_PIN);
  int soilPercent2 = constrain(map(soilRaw2, SOIL_DRY, SOIL_WET, 0, 100), 0, 100);

  // ===== SMOKE =====
  int smokeRaw = analogRead(SMOKE_PIN);
  int smokePercent = constrain(map(smokeRaw, 0, 4095, 0, 100), 0, 100);

  // ===== DHT =====
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) return;

  // ===== FIELD STATUS =====
  bool field1NeedsWater = (soilPercent1 < FIELD1_THRESHOLD);
  bool field2NeedsWater = (soilPercent2 < FIELD2_THRESHOLD);

  // ===== FIRE ALERT NOTIFICATION =====
  if (smokePercent > FIRE_THRESHOLD) {
    digitalWrite(PUMP_PIN, LOW);  // Pump ON
    digitalWrite(BUZZER_PIN, HIGH);
    Blynk.virtualWrite(V5, 1);    // Pump status

    // Send independent fire notification
    if (!fireAlertSent && millis() - lastFireAlertTime > fireCooldown) {
      String fireMessage = "🔥 FIRE ALERT!\n";
      fireMessage += "Temp: " + String(temperature) + "°C\n";
      fireMessage += "Humidity: " + String(humidity) + "%\n";
      fireMessage += "Smoke: " + String(smokePercent) + "%";
      Blynk.logEvent("fire_alert", fireMessage); // Fire event
      fireAlertSent = true;
      lastFireAlertTime = millis();
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    fireAlertSent = false;
  }

  // ===== PUMP CONTROL (MANUAL / AUTO) =====
  if (!manualPump) {
    if (field1NeedsWater || field2NeedsWater) {
      digitalWrite(PUMP_PIN, LOW); // Pump ON
      Blynk.virtualWrite(V5, 1);
    } else {
      digitalWrite(PUMP_PIN, HIGH); // Pump OFF
      Blynk.virtualWrite(V5, 0);
    }
  } else {
    digitalWrite(PUMP_PIN, LOW); // Pump manual ON
    Blynk.virtualWrite(V5, 1);
  }

  // ===== PIR SENSOR =====
  Blynk.virtualWrite(V3, digitalRead(PIR_PIN));

  // ===== DASHBOARD UPDATE =====
  Blynk.virtualWrite(V0, soilPercent1);
  Blynk.virtualWrite(V8, soilPercent2);
  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  Blynk.virtualWrite(V4, smokePercent);
  Blynk.virtualWrite(V9, field1NeedsWater);
  Blynk.virtualWrite(V10, field2NeedsWater);

  // ===== REAL-TIME DATA NOTIFICATION (Every Minute) =====
  if (millis() - lastDataReportTime >= dataReportInterval) {
    String report = "📊 Smart Agriculture Data\n";
    report += "Field1 Soil: " + String(soilPercent1) + "%\n";
    report += "Field2 Soil: " + String(soilPercent2) + "%\n";
    report += "Temp: " + String(temperature) + "°C\n";
    report += "Humidity: " + String(humidity) + "%\n";
    report += "Smoke: " + String(smokePercent) + "%";

    Blynk.logEvent("data_report", report); // Independent data report
    lastDataReportTime = millis();
  }

  // ===== SERIAL DEBUG =====
  Serial.print("F1:");
  Serial.print(soilPercent1);
  Serial.print("% F2:");
  Serial.print(soilPercent2);
  Serial.print("% Temp:");
  Serial.print(temperature);
  Serial.print("C Hum:");
  Serial.print(humidity);
  Serial.print("% Smoke:");
  Serial.println(smokePercent);
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);

  pinMode(SOIL1_PIN, INPUT);
  pinMode(SOIL2_PIN, INPUT);
  pinMode(SMOKE_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(PUMP_PIN, HIGH);   // Pump OFF initially
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();
  Blynk.begin(auth, ssid, pass);

  timer.setInterval(2000L, sendSensorData); // Update every 2 seconds for dashboard
}

// -------- LOOP --------
void loop() {
  Blynk.run();
  timer.run();
}