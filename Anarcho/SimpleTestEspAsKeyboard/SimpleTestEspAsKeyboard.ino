/*
  Einfaches Testprogramm um die grundlegende Funktion als KEYBOARD HID zu testen.
  Zeichen können auf der Seriellen eingegeben werden und werden dann nach Android geschickt.

  ESP32 BLE-HID Keyboard (DE) – Boot-Protocol-First, NimBLE 2.3.6+/2.4.x
  - Serial (USB) -> BLE HID Keyboard (Android/iOS)
  - Deutsches Layout inkl. ä/ö/ü/ß/€, Z/Y
  - HID-Service 0x1812 + Boot-Keyboard (0x2A22/0x2A32)
  - Protocol Mode default = Boot (0) für Android
  - UTF-8 Decoder für seriellen Input
*/

#include <NimBLEDevice.h>
#include "src/HidKeyboard.h"

// ---------- Globals ----------
static HidKeyboard gKeyboard;

// ---------- UTF-8 Decoder ----------
struct Utf8Decoder {
    uint32_t codepoint = 0;
    uint8_t  needed = 0;
    bool feed(uint8_t byte, uint32_t& out) {
        if (needed == 0) {
            if (byte < 0x80) { out = byte; return true; }
            if ((byte & 0xE0) == 0xC0) { codepoint = byte & 0x1F; needed = 1; return false; }
            if ((byte & 0xF0) == 0xE0) { codepoint = byte & 0x0F; needed = 2; return false; }
            if ((byte & 0xF8) == 0xF0) { codepoint = byte & 0x07; needed = 3; return false; }
            return false;
        }
        else {
            if ((byte & 0xC0) != 0x80) { needed = 0; return false; }
            codepoint = (codepoint << 6) | (byte & 0x3F);
            if (--needed == 0) { out = codepoint; return true; }
            return false;
        }
    }
} gUtf;

// ---------- Setup ----------
void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("\n[BOOT] ESP32 BLE-HID Keyboard (DE) – Boot-Protocol-First");

    gKeyboard.begin();
}

// ---------- Loop ----------
void loop() {
    // 1) Serielle Eingabe -> tippen
    while (Serial.available()) {
        uint8_t b = (uint8_t)Serial.read();
        uint32_t cp;
        if (gUtf.feed(b, cp)) {
            if (cp == '\r') continue; // nur LF
            if (!gKeyboard.isConnected()) {
                if (cp <= 0x7F) Serial.printf("[WARN] Nicht verbunden: '%c' (0x%02X)\n", (char)cp, (unsigned)cp);
                else            Serial.printf("[WARN] Nicht verbunden: U+%04lX\n", (unsigned long)cp);
                continue;
            }
            gKeyboard.typeCodepoint(cp);
        }
    }

#ifdef AUTOTIPPER
    // 2) Auto-Tipper: alle 1000 ms (außerhalb der while-Schleife)
    if (gKeyboard.isConnected() && millis() - gLastAuto >= 1000) {
        gLastAuto = millis();
        uint8_t b = (uint8_t)gDemo[gDemoIdx++];
        if (b == 0) { gDemoIdx = 0; b = (uint8_t)gDemo[gDemoIdx++]; }
        uint32_t cp;
        static Utf8Decoder autoDec; // eigener Decoder
        if (autoDec.feed(b, cp)) {
            if (cp != '\r') gKeyboard.typeCodepoint(cp);
        }
    }
#endif

    delay(2);
}

