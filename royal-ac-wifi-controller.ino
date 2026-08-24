#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "src/ac_state.h"
#include "src/secret.h"

#define EMISOR_PIN 26
#define THERM_PIN 35


const float SERIES_RESISTOR = 9700.0; // 10k


// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1   // Reset pin not used with ESP32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WebServer
WebServer server(80);

ACState ac_state;
bool updated = false;
float temp, humidity;

// ================================
// SETUP
// ================================

void setup() {
  Serial.begin(115200);

  pinMode(EMISOR_PIN, OUTPUT);

  Serial.println();
  Serial.println("AC IR decoder starting...");


  ac_state = new_ac_state();

  connectWiFi();

  server.onNotFound(handle_NotFound);
  server.on("/", HTTP_GET,  handle_GetAcState);
  TODO: server.on("/", HTTP_PUT,  handle_UpdateAcState);
  server.begin();
  Serial.println("Http server started");


  // Initialize I2C with SDA=21 and SCL=22
  Wire.begin(21, 22);
  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }

  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(15, 0);
}


// ================================
// LOOP
// ================================


void loop() {
  server.handleClient();
  if (updated) generate_ir_signal();

  calculateTemperature();

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 3);
  display.print("Temp:");

  display.setCursor(30, 25);

  // Display error message if the reading was invalid
  if (temp == -1.0) {
    display.print("Err");
  } else {
    display.print(temp, 1);
    display.print("C");
  }

  display.display();
  delay(10);
}

unsigned long sample_start = 0;
uint32_t adc_sum = 0;
uint32_t adc_samples = 0;

// ================================
// WIFI
// ================================

void connectWiFi() {
  Network.begin();

  WiFi.STA.begin();
  WiFi.STA.connect(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.STA.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
}


// ================================
// Server
// ================================

void handle_GetAcState() {
  JsonDocument djb;
  String output;
  djb["power"] = ac_state.power;
  djb["temp"] = ac_state.temp;
  djb["healthy"] = ac_state.healthy;
  djb["fan_mode"] = ac_state.fan_mode;
  djb["display"] = ac_state.display;
  djb["swing"] = ac_state.swing;
  djb["super"] = ac_state.super;
  djb["room_temp"] = temp;
  djb["room_humidity"] = humidity;
  serializeJson(djb, output);
  server.send(200, "application/json" , output);
}

void handle_UpdateAcState() {
  JsonDocument req_json;
  deserializeJson(req_json, server.arg("plain"));
  // NOTE: order matters
  if (req_json["temp"].is<uint8_t>()) {
    set_temperature(&ac_state, req_json["temp"]);
  }
  if (req_json["fan_mode"].is<FanMode>()) {
    set_fan_mode(&ac_state, req_json["fan_mode"]);
  }
  // NOTE: activate super mode will override temp and fan_mode values to (temp: 16, fan_mode: AUTO)
  if (req_json["super"].is<bool>()) {
    set_super(&ac_state, req_json["super"]);
  }
  if (req_json["healthy"].is<bool>()) {
    set_healthy(&ac_state, req_json["healthy"]);
  }
  if (req_json["display"].is<bool>()) {
    set_display(&ac_state, req_json["display"]);
  }
  if (req_json["swing"].is<bool>()) {
    set_swing(&ac_state, req_json["swing"]);
  }
  if (req_json["power"].is<PowerMode>()) {
    set_power(&ac_state, req_json["power"]);
  }

  Serial.printf("%v\n", ac_state);
  updated = true;
  server.send(200, "application/json" , "{}");
}

void handle_NotFound() {
  server.send(404, "application/json" ,"{}");
}

void generate_ir_signal() {
  uint8_t* frame = build_frame(&ac_state);
  Serial.printf("Generated Frame: %v\n", frame);
  mark(3000);
  space(1500);
  for (int i = 0; i < 14; i++) {
    uint8_t current_byte = frame[i];
    for (int j = 7 ; j >= 0; j--) {
      mark(550);
      if ((current_byte >> j) & 1) {
        space(1050);
      } else {
        space(250);
      }
    }
  }

  mark(550);
  space(0);

  delete[] frame;
  updated = false;
}

void mark(uint32_t duration_us) {
    uint32_t start = micros();

    while (micros() - start < duration_us) {
        digitalWrite(EMISOR_PIN, HIGH);
        delayMicroseconds(13);

        digitalWrite(EMISOR_PIN, LOW);
        delayMicroseconds(13);
    }
}

void space(uint32_t duration_us) {
    digitalWrite(EMISOR_PIN, LOW);
    delayMicroseconds(duration_us);
}


float calculateTemperature() {
  int tempReading = 0;

  for (int i = 0; i < 40; i++) {
    int val = analogRead(THERM_PIN);
    tempReading += constrain(val, 10, 4085);
    delay(3);
  }

  tempReading /= 40;

  float voltage = tempReading * 3.3 / 4095.0;
  voltage *= 1.09;

  float ntcResistance =
      SERIES_RESISTOR * voltage / (3.3 - voltage);
  ntcResistance *= 1.06;

  if (ntcResistance <= 0) {
    return -1.0;
  }

  const float BETA = 3220.0;
  const float T0 = 25.0 + 273.15;
  const float R0 = 9050.0;

  float tempK =
      1.0 /
      (
        (1.0 / T0) +
        (1.0 / BETA) * log(ntcResistance / R0)
      );

  Serial.print("ADC: ");
  Serial.println(tempReading);

  Serial.print("Voltage: ");
  Serial.println(voltage);

  Serial.print("NTC resistance: ");
  Serial.println(ntcResistance);

  temp = tempK - 273.15;

  Serial.print("Current Temp: ");
  Serial.println(temp);
}
