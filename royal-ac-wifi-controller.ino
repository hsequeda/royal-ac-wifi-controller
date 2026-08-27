#include <WiFi.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MQTTClient.h>
#include "src/ac_state.h"
#include "src/secret.h"

#define EMISOR_PIN 26
#define THERM_PIN 35

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1   // Reset pin not used with ESP32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

ACState ac_state;
float temp, humidity;


WiFiClient network;
MQTTClient mqtt = MQTTClient(1024);

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

  connectToMQTT();
}


// ================================
// LOOP
// ================================


void loop() {
  mqtt.loop();

  calculateTemperature();
  publish_room_temperature();

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
}

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
// MQTT
// ================================

const char DISCOVERY_CLIMATE_TOPIC[] = "homeassistant/climate/ac_oficina/config";
const char DISCOVERY_DISPLAY_SWITCH_TOPIC[] = "homeassistant/switch/ac_oficina_display/config";
const char DISCOVERY_HEALTHY_SWITCH_TOPIC[] = "homeassistant/switch/ac_oficina_healthy/config";
const char ROOM_TEMPERATURE_TOPIC[] = "ac/room_temperature";
const char TEMPERATURE_STATE_TOPIC[] = "ac/temperature/state";
const char TEMPERATURE_SET_TOPIC[] = "ac/temperature/set";
const char MODE_STATE_TOPIC[] = "ac/mode/state";
const char MODE_SET_TOPIC[] = "ac/mode/set";
const char AVAILABILITY_TOPIC[] = "ac/availability";
const char FAN_MODE_SET_TOPIC[] = "ac/fan_mode/set";
const char FAN_MODE_STATE_TOPIC[] = "ac/fan_mode/state";
const char PRESET_MODE_SET_TOPIC[] = "ac/preset_mode/set";
const char PRESET_MODE_STATE_TOPIC[] = "ac/preset_mode/state";
const char SWING_MODE_SET_TOPIC[] = "ac/swing_mode/set";
const char SWING_MODE_STATE_TOPIC[] = "ac/swing_mode/state";
const char HEALTHY_SET_TOPIC[] = "ac/healthy/set";
const char HEALTHY_STATE_TOPIC[] = "ac/healthy/state";
const char DISPLAY_SET_TOPIC[] = "ac/display/set";
const char DISPLAY_STATE_TOPIC[] = "ac/display/state";


void connectToMQTT() {
  // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADDR, MQTT_BROKER_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageHandler);

  Serial.println("Connecting to MQTT broker");

  mqtt.setWill(AVAILABILITY_TOPIC, "offline", true, 1);
  while (!mqtt.connect(MQTT_CLIENT_ID)) {
    Serial.print(".");
    delay(100);
  }

  Serial.println();

  if (!mqtt.connected()){
    Serial.println("MQTT broker Timeout!");
    return;
  }

  publish_discovery_climate();
  publish_discovery_display_switch();
  publish_discovery_healthy_switch();

  // subscriptions
  subscribe(TEMPERATURE_SET_TOPIC);
  subscribe(MODE_SET_TOPIC);
  subscribe(FAN_MODE_SET_TOPIC);
  subscribe(PRESET_MODE_SET_TOPIC);
  subscribe(SWING_MODE_SET_TOPIC);
  subscribe(DISPLAY_SET_TOPIC);
  subscribe(HEALTHY_SET_TOPIC);


  publish_availability();
  publish_mode_state();
  publish_temperature_state();
  publish_fan_mode_state();
  publish_preset_mode_state();
  publish_swing_state();
  publish_display_state();
  publish_healthy_state();
}


void publish_discovery_climate() {
  StaticJsonDocument<2048> message;
  message["name"] = "AC Oficina";
  message["unique_id"] = "ac_oficina";
  message["precision"] = "1.0";
  message["min_temp"] = "16";
  message["max_temp"] = "31";
  JsonArray modes = message.createNestedArray("modes");
  modes.add("off");
  modes.add("cool");
  JsonArray fan_modes = message.createNestedArray("fan_modes");
  fan_modes.add("auto");
  fan_modes.add("low");
  fan_modes.add("medium");
  fan_modes.add("high");
  JsonArray presets = message.createNestedArray("preset_modes");
  presets.add("Super");
  JsonArray swing_modes = message.createNestedArray("swing_modes");
  swing_modes.add("on");
  swing_modes.add("off");
  message["current_temperature_topic"] = ROOM_TEMPERATURE_TOPIC;
  message["temperature_command_topic"] = TEMPERATURE_SET_TOPIC;
  message["temperature_state_topic"] = TEMPERATURE_STATE_TOPIC;
  message["mode_command_topic"] = MODE_SET_TOPIC;
  message["mode_state_topic"] = MODE_STATE_TOPIC;
  message["availability_topic"] = AVAILABILITY_TOPIC;
  message["fan_mode_command_topic"] = FAN_MODE_SET_TOPIC;
  message["fan_mode_state_topic"] = FAN_MODE_STATE_TOPIC;
  message["preset_mode_command_topic"] = PRESET_MODE_SET_TOPIC;
  message["preset_mode_state_topic"] = PRESET_MODE_STATE_TOPIC;
  message["swing_mode_command_topic"] = SWING_MODE_SET_TOPIC;
  message["swing_mode_state_topic"] = SWING_MODE_STATE_TOPIC;

  JsonObject device = message.createNestedObject("device");
  device["name"] = "AC Oficina";
  device["manufacturer"] = "Custom";
  device["model"] = "Royal IR Controller";
  JsonArray identifiers = device.createNestedArray("identifiers");
  identifiers.add("ac_oficina");


  char messageBuffer[2048];
  serializeJson(message, messageBuffer);
  mqtt.publish(DISCOVERY_CLIMATE_TOPIC, messageBuffer, true, 0);
}

void publish_discovery_display_switch() {
  StaticJsonDocument<512> message;
  message["name"] = "Display";
  message["unique_id"] = "ac_oficina_display";
  message["payload_on"] = "ON";
  message["payload_off"] = "OFF";
  message["availability_topic"] = AVAILABILITY_TOPIC;
  message["command_topic"] = DISPLAY_SET_TOPIC;
  message["state_topic"] = DISPLAY_STATE_TOPIC;

  JsonObject device = message.createNestedObject("device");
  device["name"] = "AC Oficina";
  device["manufacturer"] = "Custom";
  device["model"] = "Royal IR Controller";
  JsonArray identifiers = device.createNestedArray("identifiers");
  identifiers.add("ac_oficina");

  char messageBuffer[512];
  serializeJson(message, messageBuffer);
  mqtt.publish(DISCOVERY_DISPLAY_SWITCH_TOPIC, messageBuffer, true, 0);
}

void publish_discovery_healthy_switch() {
  StaticJsonDocument<512> message;
  message["name"] = "Healthy";
  message["unique_id"] = "ac_oficina_healthy";
  message["payload_on"] = "ON";
  message["payload_off"] = "OFF";
  message["availability_topic"] = AVAILABILITY_TOPIC;
  message["command_topic"] = HEALTHY_SET_TOPIC;
  message["state_topic"] = HEALTHY_STATE_TOPIC;

  JsonObject device = message.createNestedObject("device");
  device["name"] = "AC Oficina";
  device["manufacturer"] = "Custom";
  device["model"] = "Royal IR Controller";
  JsonArray identifiers = device.createNestedArray("identifiers");
  identifiers.add("ac_oficina");

  char messageBuffer[512];
  serializeJson(message, messageBuffer);
  mqtt.publish(DISCOVERY_HEALTHY_SWITCH_TOPIC, messageBuffer, true, 0);
}

void subscribe(const char topic[]) {
  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(topic))
    Serial.print("Subscribed to the topic: ");
  else
    Serial.print("Failed to subscribe to the topic: ");

  Serial.println(topic);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("received from MQTT:");
  Serial.println("- topic: " + topic);
  Serial.println("- payload: " + payload);

  if (topic == MODE_SET_TOPIC)
    handle_mode_set(payload);
  else if (topic == TEMPERATURE_SET_TOPIC)
    handle_temperature_set(payload);
  else if (topic == FAN_MODE_SET_TOPIC)
    handle_fan_mode_set(payload);
  else if (topic == PRESET_MODE_SET_TOPIC)
    handle_preset_mode_set(payload);
  else if (topic == SWING_MODE_SET_TOPIC)
    handle_swing_set(payload);
  else if (topic == DISPLAY_SET_TOPIC)
    handle_display_set(payload);
  else if (topic == HEALTHY_SET_TOPIC)
    handle_healthy_set(payload);
}

void handle_mode_set(String &payload) {
  bool updated = false;
  if (payload == "off") {
      set_power(&ac_state, POWER_TO_OFF);
      updated = true;
  } else if (payload == "cool") {
      set_power(&ac_state, POWER_ON);
      updated = true;
  }

  if (updated) {
    publish_mode_state();
    generate_ir_signal();
  }
}

void handle_temperature_set(String &payload) {
  set_temperature(&ac_state, payload.toInt());
  publish_temperature_state();
  publish_mode_state();
  publish_preset_mode_state();
  generate_ir_signal();
}

void handle_fan_mode_set(String &payload) {
  Serial.println("Fan Mode: " + payload);
  if (payload == "low")
    set_fan_mode(&ac_state, FAN_LOW);
  else if (payload == "medium")
    set_fan_mode(&ac_state, FAN_MIDDLE);
  else if (payload == "high")
    set_fan_mode(&ac_state, FAN_HIGH);
  else
    set_fan_mode(&ac_state, FAN_AUTO);

  Serial.println(ac_state.fan_mode);

  publish_fan_mode_state();
  publish_preset_mode_state();
  generate_ir_signal();
}

void handle_preset_mode_set(String &payload) {
  Serial.println("Preset Mode: " + payload);
  set_super(&ac_state, payload == "Super");

  publish_preset_mode_state();
  publish_temperature_state();
  publish_fan_mode_state();
  generate_ir_signal();
}

void handle_swing_set(String &payload) {
  set_swing(&ac_state, payload == "on");
  publish_swing_state();
  generate_ir_signal();
}

void handle_display_set(String &payload) {
  set_display(&ac_state, payload == "ON");
  publish_display_state();
  generate_ir_signal();
}

void handle_healthy_set(String &payload) {
  set_healthy(&ac_state, payload == "ON");
  publish_healthy_state();
  generate_ir_signal();
}

void publish_temperature_state() {
  mqtt.publish(TEMPERATURE_STATE_TOPIC, String(ac_state.temp), true, 0);
}

void publish_fan_mode_state() {
  switch (ac_state.fan_mode) {
    case FAN_AUTO:
      mqtt.publish(FAN_MODE_STATE_TOPIC, "auto", true, 0);
      break;
    case FAN_LOW:
      mqtt.publish(FAN_MODE_STATE_TOPIC, "low", true, 0);
      break;
    case FAN_MIDDLE:
      mqtt.publish(FAN_MODE_STATE_TOPIC, "medium", true, 0);
      break;
    case FAN_HIGH:
      mqtt.publish(FAN_MODE_STATE_TOPIC, "high", true, 0);
      break;
  }
}

void publish_preset_mode_state() {
  if (ac_state.super)
    mqtt.publish(PRESET_MODE_STATE_TOPIC, "Super", true, 0);
  else
    mqtt.publish(PRESET_MODE_STATE_TOPIC, "None", true, 0);
}

void publish_mode_state() {
  switch (ac_state.power) {
    case POWER_TO_ON:
      mqtt.publish(MODE_STATE_TOPIC, "cool", true, 0);
      break;
    case POWER_TO_OFF:
      mqtt.publish(MODE_STATE_TOPIC, "off", true, 0);
      break;
    case POWER_ON:
      mqtt.publish(MODE_STATE_TOPIC, "cool", true, 0);
      break;
  }
}

void publish_swing_state() {
  if (ac_state.swing)
      mqtt.publish(SWING_MODE_STATE_TOPIC, "on", true, 0);
  else
      mqtt.publish(SWING_MODE_STATE_TOPIC, "off", true, 0);
}

void publish_display_state() {
  if (ac_state.display)
      mqtt.publish(DISPLAY_STATE_TOPIC, "ON", true, 0);
  else
      mqtt.publish(DISPLAY_STATE_TOPIC, "OFF", true, 0);
}

void publish_healthy_state() {
  if (ac_state.healthy)
      mqtt.publish(HEALTHY_STATE_TOPIC, "ON", true, 0);
  else
      mqtt.publish(HEALTHY_STATE_TOPIC, "OFF", true, 0);
}

void publish_availability() {
  mqtt.publish(AVAILABILITY_TOPIC, "online", true, 1);
}

void publish_room_temperature() {
  static uint32_t last_published = 0;
  if (millis() - last_published < 10000) return;

  last_published = millis();

  mqtt.publish(ROOM_TEMPERATURE_TOPIC, String(temp), true, 0);
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


void calculateTemperature() {
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
      9700.0 * voltage / (3.3 - voltage);
  ntcResistance *= 1.06;

  if (ntcResistance <= 0) return;

  const float BETA = 3220.0;
  const float T0 = 25.0 + 273.15;
  const float R0 = 9050.0;

  float tempK =
      1.0 /
      (
        (1.0 / T0) +
        (1.0 / BETA) * log(ntcResistance / R0)
      );

  // Serial.print("ADC: ");
  // Serial.println(tempReading);

  // Serial.print("Voltage: ");
  // Serial.println(voltage);

  // Serial.print("NTC resistance: ");
  // Serial.println(ntcResistance);

  temp = tempK - 273.15;

  // Serial.print("Current Temp: ");
  // Serial.println(temp);
}
