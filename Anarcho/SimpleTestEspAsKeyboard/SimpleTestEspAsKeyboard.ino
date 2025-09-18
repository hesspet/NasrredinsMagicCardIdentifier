/*
  ESP32 BLE-HID Keyboard Testbridge (Serial -> Smartphone), DE-Layout
  - Arduino IDE, NimBLE-Arduino
  - Sendet Zeichen aus der seriellen Konsole als Tastaturanschläge
  - DE-Mapping: ä, ö, ü, ß, € sowie Z/Y-Tausch berücksichtigt
  - Android & iOS (Bonding aktiviert)

  Baudrate im seriellen Monitor: 115200
*/

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>

static NimBLEHIDDevice* hid;
static NimBLECharacteristic* inputReport;  // Keyboard input report (ID 1)
static NimBLEAdvertising* advertising;

static volatile bool deviceConnected = false;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        deviceConnected = true;
        Serial.println("[BLE] Verbunden");
    }
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        deviceConnected = false;
        Serial.printf("[BLE] Getrennt (reason=%d). Advertising neu starten...\n", reason);
        NimBLEDevice::startAdvertising();
    }
};

// Report: [0]=Mods, [1]=reserved, [2..7]=Keycodes
static void sendReport(uint8_t mods, uint8_t k1 = 0, uint8_t k2 = 0, uint8_t k3 = 0, uint8_t k4 = 0, uint8_t k5 = 0, uint8_t k6 = 0) {
    if (!deviceConnected || !inputReport) return;
    uint8_t report[8] = { mods, 0x00, k1, k2, k3, k4, k5, k6 };
    inputReport->setValue(report, sizeof(report));
    inputReport->notify();
}
static void keyPress(uint8_t mods, uint8_t key) { sendReport(mods, key); }
static void keyReleaseAll() { sendReport(0x00, 0, 0, 0, 0, 0, 0); }

// Modifiers
#ifndef KEYBOARD_MODIFIER_RIGHTALT
#define KEYBOARD_MODIFIER_RIGHTALT 0x40
#endif

// ---------- DE-Layout Mapping ----------
// Hinweise:
// - HID-Keycodes sind Positionscodes; die Host-Layout-Einstellung "Deutsch" macht daraus die richtigen Zeichen.
// - Z/Y: In DE ergibt HID_KEY_Z -> 'y' und HID_KEY_Y -> 'z'. Wir mappen ASCII 'z' -> HID_KEY_Y und 'y' -> HID_KEY_Z.
// - Umlaute auf DE-Layout:
//     ä : HID_KEY_APOSTROPHE        (Shift => Ä)
//     ö : HID_KEY_SEMICOLON         (Shift => Ö)
//     ü : HID_KEY_LEFT_BRACKET      (Shift => Ü)
//     ß : HID_KEY_MINUS             (Shift => ẞ auf modernen Systemen; andernfalls '?')
// - € : AltGr (Right Alt) + E
// - Viele Satzzeichen bleiben US-basiert und funktionieren in der Praxis ausreichend für Tests.

static bool asciiToHid_DE(char c, uint8_t& mods, uint8_t& key) {
    mods = 0;

    // Zeilenende / Steuerzeichen
    if (c == '\n' || c == '\r') { key = HID_KEY_ENTER; return true; }
    if (c == '\t') { key = HID_KEY_TAB; return true; }
    if (c == ' ') { key = HID_KEY_SPACEBAR; return true; }

    // Ziffern
    if (c >= '1' && c <= '9') { key = HID_KEY_1 + (c - '1'); return true; }
    if (c == '0') { key = HID_KEY_0; return true; }

    // Buchstaben (DE: Z/Y tauschen)
    if (c >= 'a' && c <= 'z') {
        if (c == 'z') { key = HID_KEY_Y; return true; }
        if (c == 'y') { key = HID_KEY_Z; return true; }
        key = HID_KEY_A + (c - 'a');
        return true;
    }
    if (c >= 'A' && c <= 'Z') {
        if (c == 'Z') { key = HID_KEY_Y; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; }
        if (c == 'Y') { key = HID_KEY_Z; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; }
        key = HID_KEY_A + (c - 'A');
        mods = KEYBOARD_MODIFIER_LEFTSHIFT;
        return true;
    }

    // Umlaute / Sonderzeichen (DE)
    switch (c) {
    case 'ä': key = HID_KEY_APOSTROPHE; return true;
    case 'Ä': key = HID_KEY_APOSTROPHE; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 'ö': key = HID_KEY_SEMICOLON;  return true;
    case 'Ö': key = HID_KEY_SEMICOLON;  mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 'ü': key = HID_KEY_LEFT_BRACKET; return true;
    case 'Ü': key = HID_KEY_LEFT_BRACKET; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 'ß': key = HID_KEY_MINUS; return true;
    case '€': mods = KEYBOARD_MODIFIER_RIGHTALT; key = HID_KEY_E; return true; // AltGr+E
    }

    // Basale Interpunktion (für Tests ausreichend)
    switch (c) {
    case '.': key = HID_KEY_PERIOD; return true;
    case ',': key = HID_KEY_COMMA; return true;

        // Minus/Underscore: auf DE sind die Positionen anders; für Tests:
    case '-': key = HID_KEY_SLASH; return true;                 // ergibt häufig '-' auf DE
    case '_': key = HID_KEY_SLASH; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;

    case '+': key = HID_KEY_RIGHT_BRACKET; return true;         // '+' auf DE
    case '*': key = HID_KEY_RIGHT_BRACKET; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;

    case ':': key = HID_KEY_PERIOD; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;  // häufig Shift+'.'
    case ';': key = HID_KEY_COMMA; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;   // häufig Shift+','

    case '!': key = HID_KEY_1; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '?': key = HID_KEY_MINUS; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;   // oft Shift+'ß'
    case '=': key = HID_KEY_EQUAL; return true;
    case '/': key = HID_KEY_7; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;        // AltGr+7 -> '/'
    case '\\': key = HID_KEY_MINUS; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;   // AltGr+ß -> '\'
    case '#': key = HID_KEY_BACKSLASH; return true;                                   // DE: '#' auf <Backslash>-Position
    case '"': key = HID_KEY_2; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;       // Shift+2 -> "
    case '\'': key = HID_KEY_BACKSLASH; mods = KEYBOARD_MODIFIER_RIGHTALT; return true; // AltGr+<Backslash> -> '
    case '(': key = HID_KEY_8; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case ')': key = HID_KEY_9; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '[': key = HID_KEY_8; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;        // AltGr+8 -> [
    case ']': key = HID_KEY_9; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;        // AltGr+9 -> ]
    case '{': key = HID_KEY_7; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;        // AltGr+7 -> {
    case '}': key = HID_KEY_0; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;        // AltGr+0 -> }
    case '<': key = HID_KEY_NON_US_BACKSLASH; return true;                             // DE: eigene Taste links von Y (<>|): '<'
    case '>': key = HID_KEY_NON_US_BACKSLASH; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; // '>'
    case '|': key = HID_KEY_NON_US_BACKSLASH; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;  // AltGr+<> -> '|'
    }

    return false; // nicht gemappt
}

static void typeChar(char c) {
    uint8_t mods, key;
    if (!asciiToHid_DE(c, mods, key)) {
        Serial.printf("[HID] Unmapped: 0x%02X '%c'\n", (unsigned char)c, (c >= 32 && c < 127) ? c : '.');
        return;
    }
    keyPress(mods, key);
    delay(10);
    keyReleaseAll();
    delay(5);
}

// Standard 8-Byte Keyboard Report Map w/ LEDs
static const uint8_t KEYBOARD_REPORTMAP[] = {
  USAGE_PAGE(1), 0x01,       // Generic Desktop
  USAGE(1),      0x06,       // Keyboard
  COLLECTION(1), 0x01,       // Application
    REPORT_ID(1), 0x01,
    USAGE_PAGE(1), 0x07,     // Keyboard/Keypad
    USAGE_MINIMUM(1), 0xE0,
    USAGE_MAXIMUM(1), 0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1), 0x01,
    REPORT_COUNT(1), 0x08,   // Modifiers
    INPUT(1), 0x02,          // Data,Var,Abs
    REPORT_COUNT(1), 0x01,
    REPORT_SIZE(1), 0x08,
    INPUT(1), 0x01,          // Const
    REPORT_COUNT(1), 0x05,
    REPORT_SIZE(1), 0x01,
    USAGE_PAGE(1), 0x08,     // LEDs
    USAGE_MINIMUM(1), 0x01,
    USAGE_MAXIMUM(1), 0x05,
    OUTPUT(1), 0x02,         // Data,Var,Abs
    REPORT_COUNT(1), 0x01,
    REPORT_SIZE(1), 0x03,
    OUTPUT(1), 0x01,         // Const
    REPORT_COUNT(1), 0x06,
    REPORT_SIZE(1), 0x08,
    USAGE_PAGE(1), 0x07,
    USAGE_MINIMUM(1), 0x00,
    USAGE_MAXIMUM(1), 0x65,
    INPUT(1), 0x00,          // Data,Array,Abs
  END_COLLECTION(0)
};

void setup() {
    Serial.begin(115200);
    while (!Serial) { /* USB wait */ }
    Serial.println("\n[BOOT] ESP32 BLE-HID Keyboard (DE) startet...");

    NimBLEDevice::init("ESP32 DE Keyboard");
    NimBLEDevice::setPower(ESP_PWR_LVL_P7);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT); // Just Works
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityAuth(true, true, true); // bonding + secure connection

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    hid = new NimBLEHIDDevice(server);
    hid->manufacturer()->setValue("TestCo");
    hid->pnp(0x02, 0xCAFE, 0x4001, 0x0100);
    hid->hidInfo(0x00, 0x01);
    hid->reportMap((uint8_t*)KEYBOARD_REPORTMAP, sizeof(KEYBOARD_REPORTMAP));
    hid->setBatteryLevel(100);
    hid->startServices();

    inputReport = hid->inputReport(1);

    advertising = NimBLEDevice::getAdvertising();
    advertising->setAppearance(HID_KEYBOARD);
    advertising->addServiceUUID(hid->hidService()->getUUID());
    advertising->start();

    Serial.println("[BLE] Advertising als HID-Tastatur. Koppel Dein Smartphone (DE-Layout auf dem Phone empfohlen).");
    Serial.println("[INFO] Tippe Text im seriellen Monitor. Enter sendet ENTER.");
}

void loop() {
    while (Serial.available()) {
        char c = (char)Serial.read();

        // CRLF handhaben: bare '\r' ignorieren, '\n' löst ENTER aus
        if (c == '\r') continue;

        if (!deviceConnected) {
            Serial.printf("[WARN] Noch nicht verbunden: '%c' (0x%02X)\n", (c >= 32 && c < 127) ? c : '.', (unsigned char)c);
            continue;
        }
        typeChar(c);
    }
    delay(2);
}
