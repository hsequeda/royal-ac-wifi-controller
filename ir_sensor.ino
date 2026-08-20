#include <Network.h>
#include <WiFi.h>

#define IR_PIN 23
#define EMISOR_PIN 26


const uint8_t BASE_FRAME [14] = {
  0xC4, 0xD3, 0x64, 0x80, 0x00, 0x24, 0xCA,
  0xF0, 0xBC, 0x00, 0x00, 0x00, 0x01, 0x1A
};

enum PowerMode {
  TO_ON,
  TO_OFF,
  ON
};

enum FanMode {
  FAN_AUTO,
  FAN_LOW,
  FAN_MIDDLE,
  FAN_HIGH,
};

struct ACState {
  PowerMode power;
  uint8_t temp;
  bool healthy;
  FanMode fan_mode;
  bool display;
  bool swing;
  bool super;
};

// ================================
// WIFI
// ================================

const char* WIFI_SSID     = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASS";

// Telnet = TCP puerto 23
NetworkServer telnetServer(23);
NetworkClient telnetClient;


// ================================
// OUTPUT: SERIAL + TELNET
// ================================

void consolePrint(const char* text) {
  Serial.print(text);

  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(text);
  }
}

void consolePrintln(const char* text) {
  Serial.println(text);

  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(text);
  }
}

void consolePrintHexByte(uint8_t value) {

  if (value < 0x10) {
    Serial.print("0");

    if (telnetClient && telnetClient.connected()) {
      telnetClient.print("0");
    }
  }

  Serial.print(value, HEX);

  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(value, HEX);
  }
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

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.STA.localIP());

  telnetServer.begin();

  Serial.println("Telnet server started");
  Serial.print("Connect using: telnet ");
  Serial.print(WiFi.STA.localIP());
  Serial.println(" 23");
}


// ================================
// TELNET CLIENT
// ================================

void handleTelnet() {

  // Check for new connection
  NetworkClient newClient = telnetServer.accept();

  if (newClient) {

    // Only one Telnet client at a time
    if (telnetClient && telnetClient.connected()) {
      telnetClient.stop();
    }

    telnetClient = newClient;

    Serial.println("Telnet client connected");

    telnetClient.println();
    telnetClient.println("==========================");
    telnetClient.println(" ESP32 AC IR DECODER");
    telnetClient.println("==========================");
    telnetClient.println();
  }

  // Optionally read anything sent through Telnet
  if (telnetClient && telnetClient.connected()) {

    while (telnetClient.available()) {

      char c = telnetClient.read();

      // For now just mirror it to USB Serial
      Serial.write(c);
    }
  }
}


// ================================
// IR PULSE
// ================================

unsigned long readPulse(
  int level,
  unsigned long timeout = 30000
) {

  unsigned long start = micros();

  while (digitalRead(IR_PIN) == level) {

    if (micros() - start > timeout) {
      return 0;
    }
  }

  return micros() - start;
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
  // generate_ir_signal();
  handleTelnet();

  handle_ac_signal();
}


// ================================
// AC DECODER
// ================================

void handle_ac_signal() {

  // Wait for LOW header
  if (digitalRead(IR_PIN) != LOW) {
    return;
  }

  // ==============================
  // HEADER LOW
  // ==============================

  unsigned long lowTime = readPulse(LOW);

  if (lowTime < 2500 || lowTime > 4000) {
    return;
  }


  // ==============================
  // HEADER HIGH
  // ==============================

  unsigned long highTime = readPulse(HIGH);

  if (highTime < 1400 || highTime > 1700) {
    return;
  }


  // ==============================
  // 112 bits = 14 bytes
  // ==============================

  uint8_t code[14] = {0};


  for (int i = 0; i < 112; i++) {

    // Ewery bit starts with LOW
    unsigned long bitLow = readPulse(LOW);

    if (bitLow < 400 || bitLow > 700) {
      return;
    }


    // HIGH determines 0 or 1
    unsigned long bitHigh = readPulse(HIGH);

    if (bitHigh == 0) {
      return;
    }


    // Which byte are we currently filling?
    int byteIndex = i / 8;


    // Shift current byte
    code[byteIndex] <<= 1;


    // Based on your captured signal:
    //
    // HIGH short ≈ 150 - 350 us
    // HIGH long  ≈ 1030 - 1080 us
    //
    // 700 us gives us plenty of separation

    if (bitHigh > 700) {
      code[byteIndex] |= 1;
    }
  }


  // ==============================
  // MESSAGE RECEIVED
  // ==============================

  consolePrintln("");
  consolePrintln("===== AC MESSAGE =====");

  consolePrint("HEX: ");

  for (int i = 0; i < 14; i++) {

    consolePrintHexByte(code[i]);

    consolePrint(" ");
  }

  consolePrintln("");
  consolePrintln("======================");
}

void generate_ir_signal(ACState ac_state) {
  uint8_t* x = build_frame(ac_state);
  mark(3000);
  space(1500);
  for (int i = 0; i < 14; i++) {
    uint8_t val = BASE_FRAME[i];
    for (int j = 7 ; j >= 0; j--) {
      mark(550);
      if ((val >> j) & 1) {
        space(1050);
      } else {
        space(250);
      }
    }
  }

  mark(550);
  space(0);

  delete[] x;
}

uint8_t* build_frame(ACState ac_state) {
  uint8_t* arr = new uint8_t[14];
  for (uint8_t i = 0; i < 14; i++) {
    arr[i] = BASE_FRAME[i];
  }

  // Handle power
  switch (ac_state.power) {
    case TO_ON:
      arr[5] &= ~(1 << 5);
      arr[8] |= (1 << 1);
    case TO_OFF:
      arr[5] |= (1 << 5);
      arr[8] |= (1 << 1);
    default:
      arr[5] |= (1 << 5);
  }

  // Handle super
  arr[6] &= ~(1 << 1); // disable as default
  // super also enable swing, change temp to 16 and set fan in auto
  // it also activates the swing, but it's not necessary, a super state can live with swing off
  // if enable then activate the bit
  if (ac_state.healthy) {
    arr[6] |= (1 << 1);
    return arr;
  }

  // handle temp
  arr[7] = encode_temp(ac_state.temp) << 4 | (arr[7] & 0x0F);

  // Handle Healthy
  arr[6] &= ~(1 << 3); // disable as default
  // if enable then activate the bit
  if (ac_state.healthy) {
    arr[6] |= (1 << 3 );
  }

  // Handle fan_mode
  switch (ac_state.fan_mode) {
    case FAN_LOW:
      arr[8] &= ~(1 << 7); // set first bit to 0
      arr[8] |= (1 << 6); // set second bit to 1
      arr[8] &= ~(1 << 5); // set third bit to 0
    case FAN_MIDDLE:
      arr[8] |= (1 << 7); // set first bit to 1
      arr[8] |= (1 << 6); // set second bit to 1
      arr[8] &= ~(1 << 5); // set third bit to 0
    case FAN_HIGH:
      arr[8] |= (1 << 7); // set first bit to 1
      arr[8] &= ~(1 << 6); // set second bit to 0
      arr[8] |= (1 << 5); // set third bit to 1
    default:
      arr[8] &= ~(1 << 7); // set first bit to 0
      arr[8] &= ~(1 << 6); // set first bit to 0
      arr[8] &= ~(1 << 5); // set third bit to 0
  }

  // Handle display
  // set off by default
  arr[5] |= (1 << 1);
  if (ac_state.display) {
    arr[5] &= ~(1 << 1);
  }

  // Handle swing
  // set off by default
  arr[8] &= ~(1 << 2);  // set 4th bit to 0
  arr[8] &= ~(1 << 3);  // set 5th bit to 0
  arr[8] &= ~(1 << 4);  // set 6th bit to 0
  if (ac_state.swing) {
    arr[8] |= (1 << 2);  // set 4th bit to 1
    arr[8] |= (1 << 3);  // set 5th bit to 1
    arr[8] |= (1 << 4);  // set 6th bit to 1
  }

  // build checksum
  return arr;
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

uint8_t encode_temp(uint8_t temp) {
  uint8_t diff = (31 - temp) & 0x0F;
  return (diff & 0b0001) << 3 |
    (diff & 0b0010) << 1 |
    (diff & 0b0100) >> 1 |
    (diff & 0b1000) >> 3;
}
