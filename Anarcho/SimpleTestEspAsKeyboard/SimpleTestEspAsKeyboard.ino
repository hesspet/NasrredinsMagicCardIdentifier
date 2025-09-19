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
#include "src/Helper/Utf8Decoder.h"

static HidKeyboard gKeyboard;

void setup() {
	Serial.begin(115200);
	while (!Serial) {}

	Serial.println("\n[BOOT] ESP32 BLE-HID Keyboard (DE) – Boot-Protocol-First");

	gKeyboard.begin();
}

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

	delay(2);
}

