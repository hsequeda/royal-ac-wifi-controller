#include <WiFi.h>
#include <WebServer.h>

#define IR_PIN 23
#define EMISOR_PIN 26

// ================================
// WIFI
// ================================

const char* WIFI_SSID     = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASS";

// WebServer
WebServer server(80);

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


  // server.
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
}


// ================================
// SETUP
// ================================

void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);
  pinMode(EMISOR_PIN, OUTPUT);

  Serial.println();
  Serial.println("AC IR decoder starting...");

  connectWiFi();
}


// ================================
// LOOP
// ================================

void loop() {
}

// void generate_ir_signal(ACState ac_state) {
//   uint8_t* x = build_frame(ac_state);
//   mark(3000);
//   space(1500);
//   for (int i = 0; i < 14; i++) {
//     uint8_t val = BASE_FRAME[i];
//     for (int j = 7 ; j >= 0; j--) {
//       mark(550);
//       if ((val >> j) & 1) {
//         space(1050);
//       } else {
//         space(250);
//       }
//     }
//   }
//
//   mark(550);
//   space(0);
//
//   delete[] x;
// }

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
