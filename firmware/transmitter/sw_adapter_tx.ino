/*
 * ============================================================
 *  STEERING WHEEL ADAPTER — TRANSMITTER
 *  Board:    ESP32 WROOM-32 (on steering wheel)
 *  Role:     Reads physical buttons, sends state via ESP-NOW
 *  Author:   Cosmin Leonardo Cozaciuc
 *  Version:  1.0
 * ============================================================
 *
 *  WIRING:
 *   Button 1 (Volume +)   → GPIO 4  (INPUT_PULLUP)
 *   Button 2 (Volume -)   → GPIO 5  (INPUT_PULLUP)
 *   Button 3 (Next track) → GPIO 18 (INPUT_PULLUP)
 *   Button 4 (Prev track) → GPIO 19 (INPUT_PULLUP)
 *   Button 5 (Mute)       → GPIO 21 (INPUT_PULLUP)
 *   12V car               → AMS1117-3.3V → ESP32 3.3V pin
 *   GND                   → GND
 *
 *  NOTES:
 *   - Buttons connect GPIO to GND when pressed (active LOW)
 *   - INPUT_PULLUP enables internal ~45kΩ pull-up
 *   - Receiver MAC address must be set before flashing
 *   - Send interval: 20ms (50Hz) — non-blocking via millis()
 * ============================================================
 */

#include <esp_now.h>
#include <WiFi.h>

// ── RECEIVER MAC ADDRESS ─────────────────────────────────
// Get from receiver board: Serial.println(WiFi.macAddress())
uint8_t receiverMAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

// ── BUTTON PINS ──────────────────────────────────────────
const int BUTTON_PINS[] = {4, 5, 18, 19, 21};
const int NUM_BUTTONS   = 5;

// ── TIMING ───────────────────────────────────────────────
const unsigned long SEND_INTERVAL = 20; // ms — 50Hz update rate
unsigned long lastSend = 0;

// ── PACKET STRUCTURE ─────────────────────────────────────
typedef struct {
  uint8_t buttonState;  // bitmask: bit0=btn1, bit1=btn2 ...
  uint8_t checksum;     // XOR integrity check
} ButtonPacket;

esp_now_peer_info_t peerInfo;

// ── SEND CALLBACK ────────────────────────────────────────
void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  // Uncomment for debug:
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// ════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_send_cb(onDataSent);

  // Register receiver peer
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // Button pins — active LOW with internal pull-up
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  Serial.println("Transmitter ready");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

// ════════════════════════════════════════════════════════
void loop() {
  // Non-blocking send — no delay()
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    // Build button bitmask
    ButtonPacket packet = {0, 0};
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if (digitalRead(BUTTON_PINS[i]) == LOW) {
        packet.buttonState |= (1 << i);
      }
    }

    // XOR checksum for basic integrity validation on receiver
    packet.checksum = packet.buttonState ^ 0xAA;

    esp_now_send(receiverMAC, (uint8_t *)&packet, sizeof(packet));
  }
}
