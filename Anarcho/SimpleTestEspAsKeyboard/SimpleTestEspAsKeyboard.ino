/*
  ESP32 BLE-HID Keyboard (DE) – NimBLE 2.4.6, HID-Service "per Hand"
  - Serial -> BLE HID Keyboard (Android/iOS)
  - Deutsches Layout (Z/Y, ä/ö/ü/ß/€, Grundzeichen)
*/

#include <NimBLEDevice.h>

// --------- HID Keycodes / Modifier Fallbacks (falls Header fehlen) ---------
#ifndef HID_KEY_ENTER
#define HID_KEY_A              0x04
#define HID_KEY_B              0x05
#define HID_KEY_C              0x06
#define HID_KEY_D              0x07
#define HID_KEY_E              0x08
#define HID_KEY_F              0x09
#define HID_KEY_G              0x0A
#define HID_KEY_H              0x0B
#define HID_KEY_I              0x0
#define HID_KEY_J              0x0D
#define HID_KEY_K              0x0E
#define HID_KEY_L              0x0F
#define HID_KEY_M              0x10
#define HID_KEY_N              0x11
#define HID_KEY_O              0x12
#define HID_KEY_P              0x13
#define HID_KEY_Q              0x14
#define HID_KEY_R              0x15
#define HID_KEY_S              0x16
#define HID_KEY_T              0x17
#define HID_KEY_U              0x18
#define HID_KEY_V              0x19
#define HID_KEY_W              0x1A
#define HID_KEY_X              0x1B
#define HID_KEY_Y              0x1C
#define HID_KEY_Z              0x1D
#define HID_KEY_1              0x1E
#define HID_KEY_2              0x1F
#define HID_KEY_3              0x20
#define HID_KEY_4              0x21
#define HID_KEY_5              0x22
#define HID_KEY_6              0x23
#define HID_KEY_7              0x24
#define HID_KEY_8              0x25
#define HID_KEY_9              0x26
#define HID_KEY_0              0x27
#define HID_KEY_ENTER          0x28
#define HID_KEY_ESCAPE         0x29
#define HID_KEY_BACKSPACE      0x2A
#define HID_KEY_TAB            0x2B
#define HID_KEY_SPACEBAR       0x2C
#define HID_KEY_MINUS          0x2D
#define HID_KEY_EQUAL          0x2E
#define HID_KEY_LEFT_BRACKET   0x2F
#define HID_KEY_RIGHT_BRACKET  0x30
#define HID_KEY_BACKSLASH      0x31
#define HID_KEY_SEMICOLON      0x33
#define HID_KEY_APOSTROPHE     0x34
#define HID_KEY_GRAVE          0x35
#define HID_KEY_COMMA          0x36
#define HID_KEY_PERIOD         0x37
#define HID_KEY_SLASH          0x38
#define HID_KEY_NON_US_BACKSLASH 0x64
#endif

#ifndef KEYBOARD_MODIFIER_LEFTSHIFT
#define KEYBOARD_MODIFIER_LEFTCTRL    0x01
#define KEYBOARD_MODIFIER_LEFTSHIFT   0x02
#define KEYBOARD_MODIFIER_LEFTALT     0x04
#define KEYBOARD_MODIFIER_LEFTGUI     0x08
#define KEYBOARD_MODIFIER_RIGHTCTRL   0x10
#define KEYBOARD_MODIFIER_RIGHTSHIFT  0x20
#define KEYBOARD_MODIFIER_RIGHTALT    0x40 // AltGr
#define KEYBOARD_MODIFIER_RIGHTGUI    0x80
#endif

// --------- UUIDs (BT SIG, 16-bit) ---------
static const uint16_t UUID_HID_SERVICE = 0x1812;
static const uint16_t UUID_HID_INFORMATION = 0x2A4A;
static const uint16_t UUID_REPORT_MAP = 0x2A4B;
static const uint16_t UUID_HID_CONTROL_POINT = 0x2A4C;
static const uint16_t UUID_REPORT = 0x2A4D;
static const uint16_t UUID_PROTOCOL_MODE = 0x2A4E;

// Descriptors
static const uint16_t UUID_REPORT_REFERENCE = 0x2908; // Report Reference
// CCCD (0x2902) wird von NimBLE automatisch für notify angelegt

// --------- HID Report Map (Keyboard, ReportID=1) ---------
static const uint8_t KEYBOARD_REPORTMAP[] = {
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x06,       // Usage (Keyboard)
  0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,     //   Report ID (1)
    0x05, 0x07,     //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,     //   Usage Min (LeftControl)
    0x29, 0xE7,     //   Usage Max (Right GUI)
    0x15, 0x00,     //   Logical Min (0)
    0x25, 0x01,     //   Logical Max (1)
    0x75, 0x01,     //   Report Size (1)
    0x95, 0x08,     //   Report Count (8)
    0x81, 0x02,     //   Input (Data,Var,Abs) ; Modifiers

    0x75, 0x08,     //   Report Size (8)
    0x95, 0x01,     //   Report Count (1)
    0x81, 0x01,     //   Input (Const,Array,Abs) ; Reserved

    0x05, 0x08,     //   Usage Page (LEDs)
    0x19, 0x01,     //   Usage Min (Num Lock)
    0x29, 0x05,     //   Usage Max (Kana)
    0x75, 0x01,     //   Report Size (1)
    0x95, 0x05,     //   Report Count (5)
    0x91, 0x02,     //   Output (Data,Var,Abs)

    0x75, 0x03,     //   Report Size (3)
    0x95, 0x01,     //   Report Count (1)
    0x91, 0x01,     //   Output (Const,Array,Abs)

    0x05, 0x07,     //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,     //   Usage Min (Reserved (no event))
    0x29, 0x65,     //   Usage Max (Keyboard Application)
    0x15, 0x00,     //   Logical Min (0)
    0x25, 0x01,     //   Logical Max (1)
    0x75, 0x08,     //   Report Size (8)
    0x95, 0x06,     //   Report Count (6)
    0x81, 0x00,     //   Input (Data,Array,Abs) ; 6-key rollover
  0xC0              // End Collection
};

// --------- Globale BLE-Objekte ---------
static NimBLECharacteristic* gInputReportChar = nullptr;
static NimBLEAdvertising* gAdv = nullptr;
static volatile bool         gConnected = false;

// --------- Server-Callbacks ---------
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override {
        gConnected = true;
        Serial.println("[BLE] Verbunden");
    }
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
        gConnected = false;
        Serial.printf("[BLE] Getrennt (reason=%d). Advertising neu...\n", reason);
        NimBLEDevice::startAdvertising();
    }
};

// --------- Senden eines 8-Byte Keyboard-Reports ---------
static inline void sendKeyReport(uint8_t mods,
    uint8_t k1 = 0, uint8_t k2 = 0, uint8_t k3 = 0,
    uint8_t k4 = 0, uint8_t k5 = 0, uint8_t k6 = 0) {
    if (!gConnected || !gInputReportChar) return;
    uint8_t rpt[8] = { mods, 0x00, k1, k2, k3, k4, k5, k6 };
    gInputReportChar->setValue(rpt, sizeof(rpt));
    gInputReportChar->notify();
}

static inline void pressAndRelease(uint8_t mods, uint8_t key) {
    sendKeyReport(mods, key);
    delay(8);
    sendKeyReport(0);
    delay(5);
}

// --------- DE-Mapping ---------
static bool asciiToHid_DE(char c, uint8_t& mods, uint8_t& key) {
    mods = 0;

    if (c == '\n' || c == '\r') { key = HID_KEY_ENTER; return true; }
    if (c == '\t') { key = HID_KEY_TAB; return true; }
    if (c == ' ') { key = HID_KEY_SPACEBAR; return true; }

    if (c >= '1' && c <= '9') { key = (uint8_t)(HID_KEY_1 + (c - '1')); return true; }
    if (c == '0') { key = HID_KEY_0; return true; }

    if (c >= 'a' && c <= 'z') {
        if (c == 'z') { key = HID_KEY_Y; return true; }
        if (c == 'y') { key = HID_KEY_Z; return true; }
        key = (uint8_t)(HID_KEY_A + (c - 'a')); return true;
    }
    if (c >= 'A' && c <= 'Z') {
        if (c == 'Z') { key = HID_KEY_Y; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; }
        if (c == 'Y') { key = HID_KEY_Z; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; }
        key = (uint8_t)(HID_KEY_A + (c - 'A')); mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    }

    switch (c) {
    case 'ä': key = HID_KEY_APOSTROPHE; return true;
    case 'Ä': key = HID_KEY_APOSTROPHE; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 'ö': key = HID_KEY_SEMICOLON;  return true;
    case 'Ö': key = HID_KEY_SEMICOLON;  mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 'ü': key = HID_KEY_LEFT_BRACKET; return true;
    case 'Ü': key = HID_KEY_LEFT_BRACKET; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case 'ß': key = HID_KEY_MINUS; return true;
    case '€': mods = KEYBOARD_MODIFIER_RIGHTALT; key = HID_KEY_E; return true;
    }

    switch (c) {
    case '.': key = HID_KEY_PERIOD; return true;
    case ',': key = HID_KEY_COMMA; return true;
    case '-': key = HID_KEY_SLASH; return true;
    case '_': key = HID_KEY_SLASH; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '+': key = HID_KEY_RIGHT_BRACKET; return true;
    case '*': key = HID_KEY_RIGHT_BRACKET; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case ':': key = HID_KEY_PERIOD; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case ';': key = HID_KEY_COMMA;  mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '!': key = HID_KEY_1; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '?': key = HID_KEY_MINUS; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '=': key = HID_KEY_EQUAL; return true;
    case '/': key = HID_KEY_7; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '\\': key = HID_KEY_MINUS; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '#': key = HID_KEY_BACKSLASH; return true;
    case '"': key = HID_KEY_2; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '\'': key = HID_KEY_BACKSLASH; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '(': key = HID_KEY_8; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case ')': key = HID_KEY_9; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '[': key = HID_KEY_8; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case ']': key = HID_KEY_9; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '{': key = HID_KEY_7; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '}': key = HID_KEY_0; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '<': key = HID_KEY_NON_US_BACKSLASH; return true;
    case '>': key = HID_KEY_NON_US_BACKSLASH; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    case '|': key = HID_KEY_NON_US_BACKSLASH; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    }
    return false;
}

static inline void typeChar(char c) {
    uint8_t mods, key;
    if (!asciiToHid_DE(c, mods, key)) {
        Serial.printf("[HID] Unmapped: 0x%02X '%c'\n", (unsigned char)c, (c >= 32 && c < 127) ? c : '.');
        return;
    }
    pressAndRelease(mods, key);
}

// --------- Setup: kompletter HID-Service ---------
void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("\n[BOOT] ESP32 BLE-HID Keyboard (DE) – NimBLE 2.4.6");

    NimBLEDevice::init("ESP32 DE Keyboard");
    NimBLEDevice::setPower(ESP_PWR_LVL_P6);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);   // Just Works
    NimBLEDevice::setSecurityAuth(true, false, true);            // bonding, no MITM, LESC

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    // --- HID Service (0x1812) ---
    NimBLEService* hid = server->createService((uint16_t)UUID_HID_SERVICE);

    // Protocol Mode (0 = Boot, 1 = Report). Wir nutzen Report Mode (1).
    NimBLECharacteristic* chProtocol = hid->createCharacteristic(
        (uint16_t)UUID_PROTOCOL_MODE,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
    );
    uint8_t proto = 0x01;
    chProtocol->setValue(&proto, 1);

    // HID Information (bcdHID, bCountryCode, Flags)
    // bcdHID=0x0111, country=0 (not localized), flags=0x02 (normally connectable)
    NimBLECharacteristic* chHidInfo = hid->createCharacteristic(
        (uint16_t)UUID_HID_INFORMATION,
        NIMBLE_PROPERTY::READ
    );
    uint8_t hidInfo[4] = { 0x11, 0x01, 0x00, 0x02 };
    chHidInfo->setValue(hidInfo, sizeof(hidInfo));

    // HID Control Point (host writes 0=suspend,1=exit)
    NimBLECharacteristic* chCtrlPt = hid->createCharacteristic(
        (uint16_t)UUID_HID_CONTROL_POINT,
        NIMBLE_PROPERTY::WRITE_NR
    );
    uint8_t ctl = 0x00;
    chCtrlPt->setValue(&ctl, 1);

    // Report Map
    NimBLECharacteristic* chReportMap = hid->createCharacteristic(
        (uint16_t)UUID_REPORT_MAP,
        NIMBLE_PROPERTY::READ
    );
    chReportMap->setValue((uint8_t*)KEYBOARD_REPORTMAP, sizeof(KEYBOARD_REPORTMAP));

    // Input Report (notify + read) + Report Reference Descriptor (ID=1, Type=Input)
    gInputReportChar = hid->createCharacteristic(
        (uint16_t)UUID_REPORT,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    // Report Reference: 2 Bytes -> [ReportID, ReportType(1=input,2=output,3=feature)]
    NimBLEDescriptor* repRef = gInputReportChar->createDescriptor(
        (uint16_t)UUID_REPORT_REFERENCE,
        NIMBLE_PROPERTY::READ,
        2
    );
    uint8_t repRefVal[2] = { 0x01, 0x01 }; // ID=1, Type=Input
    repRef->setValue(repRefVal, sizeof(repRefVal));

    // (Optional) Output Report (LEDs) könntest du hier ergänzen, falls benötigt.

    hid->start();

    // --- Device Information Service (optional) ---
    NimBLEService* dis = server->createService((uint16_t)0x180A);
    NimBLECharacteristic* cMan = dis->createCharacteristic((uint16_t)0x2A29, NIMBLE_PROPERTY::READ);
    cMan->setValue("TestCo");
    dis->start();

    // Advertising
    gAdv = NimBLEDevice::getAdvertising();
    gAdv->addServiceUUID((uint16_t)UUID_HID_SERVICE);
    gAdv->setAppearance(961); // HID Keyboard
    gAdv->start();

    Serial.println("[BLE] Advertising als HID Keyboard. Koppeln in den Bluetooth-Einstellungen.");
    Serial.println("[INFO] Im seriellen Monitor tippen; Enter = Zeilenumbruch.");
}

// --------- Loop: Serial -> HID ---------
void loop() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue; // CRLF: nur LF auswerten
        if (!gConnected) {
            Serial.printf("[WARN] Nicht verbunden: '%c' (0x%02X)\n", (c >= 32 && c < 127) ? c : '.', (unsigned char)c);
            continue;
        }
        typeChar(c);
    }
    delay(2);
}
