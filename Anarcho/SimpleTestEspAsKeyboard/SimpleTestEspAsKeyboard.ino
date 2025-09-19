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
#include "HidConsts.h"

// ---------- Globals ----------
static NimBLECharacteristic* gInputReport = nullptr;
static NimBLECharacteristic* gBootIn = nullptr;
static NimBLEAdvertising* gAdv = nullptr;
static volatile bool         gConnected = false;
static uint8_t               gProtocolMode = 0x00; // 0=Boot (Default), 1=Report

// Track subscriptions (CCCD)
static volatile bool gSubBootIn = false;
static volatile bool gSubReport = false;

// ---------- Protocol-Mode Callback ----------
class ProtocolModeCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) {
        std::string v = c->getValue();
        if (v.size() == 1 && (v[0] == 0x00 || v[0] == 0x01)) {
            gProtocolMode = (uint8_t)v[0];
            Serial.printf("[HID] Host set ProtocolMode = %s\n",
                gProtocolMode ? "Report(1)" : "Boot(0)");
        }
    }
};
static ProtocolModeCallbacks gProtoCb;

// ---------- Subscribe-Callbacks (beide Signaturen unterstützen) ----------
class InputSubscribeCallbacks : public NimBLECharacteristicCallbacks {
public:
    // Neuere NimBLE (NimBLEConnInfo&)
    void onSubscribe(NimBLECharacteristic* c, NimBLEConnInfo&, uint16_t subValue) {
        handleSub(c, subValue);
    }
    // Ältere NimBLE (ble_gap_conn_desc*)
    void onSubscribe(NimBLECharacteristic* c, ble_gap_conn_desc*, uint16_t subValue) {
        handleSub(c, subValue);
    }
private:
    void handleSub(NimBLECharacteristic* c, uint16_t subValue) {
        const bool notifyEnabled = (subValue & 0x0001);
        if (c == gBootIn) {
            gSubBootIn = notifyEnabled;
            Serial.printf("[HID] BootInput subscribe = %d\n", (int)gSubBootIn);
        }
        else if (c == gInputReport) {
            gSubReport = notifyEnabled;
            Serial.printf("[HID] ReportInput subscribe = %d\n", (int)gSubReport);
        }
    }
} gSubCb;

// ---------- Server-Callbacks (Signaturen ohne override) ----------
class ServerCallbacks : public NimBLEServerCallbacks {
    // Neuere NimBLE
    void onConnect(NimBLEServer*, NimBLEConnInfo&) {
        afterConnect();
    }
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int /*reason*/) {
        afterDisconnect();
    }
    // Ältere NimBLE
    void onConnect(NimBLEServer* /*s*/) {
        afterConnect();
    }
    void onDisconnect(NimBLEServer* /*s*/) {
        afterDisconnect();
    }

    void afterConnect() {
        gConnected = true;
        Serial.println("[BLE] Verbunden");
        unsigned long t0 = millis();
        while (millis() - t0 < 800) delay(1);

        auto sendRelease = []() {
            uint8_t rpt[8] = { 0,0,0,0,0,0,0,0 };
            if (gBootIn) { gBootIn->setValue(rpt, 8);      gBootIn->notify(); }
            if (gInputReport) { gInputReport->setValue(rpt, 8); gInputReport->notify(); }
            };

        uint8_t rptA[8] = { KEYBOARD_MODIFIER_LEFTSHIFT, 0, HID_KEY_A, 0,0,0,0,0 };
        uint8_t rptEnter[8] = { 0,0, HID_KEY_ENTER, 0,0,0,0,0 };

        if (gBootIn) { gBootIn->setValue(rptA, 8);      gBootIn->notify(); }
        if (gInputReport) { gInputReport->setValue(rptA, 8); gInputReport->notify(); }
        delay(10); sendRelease(); delay(10);

        if (gBootIn) { gBootIn->setValue(rptEnter, 8);      gBootIn->notify(); }
        if (gInputReport) { gInputReport->setValue(rptEnter, 8); gInputReport->notify(); }
        delay(10); sendRelease();

        Serial.printf("[HID] ProtocolMode on connect (may change) = %s\n",
            gProtocolMode ? "Report(1)" : "Boot(0)");
    }

    void afterDisconnect() {
        gConnected = false;
        gSubBootIn = gSubReport = false;
        Serial.println("[BLE] Getrennt. Advertising neu...");
        NimBLEDevice::startAdvertising();
    }
};

// ---------- Send ----------
static inline void sendKeyReportRaw(NimBLECharacteristic* ch,
    const char* tag,
    uint8_t mods,
    uint8_t k1 = 0, uint8_t k2 = 0, uint8_t k3 = 0,
    uint8_t k4 = 0, uint8_t k5 = 0, uint8_t k6 = 0) {
    if (!ch) return;
    uint8_t rpt[8] = { mods, 0x00, k1, k2, k3, k4, k5, k6 };
    ch->setValue(rpt, sizeof(rpt));
    bool ok = ch->notify();
    Serial.printf("[HID] send %s notify=%d mods=%02X keys=%02X %02X %02X %02X %02X %02X\n",
        tag, (int)ok, mods, k1, k2, k3, k4, k5, k6);
}

static inline void sendKeyReport(uint8_t mods,
    uint8_t k1 = 0, uint8_t k2 = 0, uint8_t k3 = 0,
    uint8_t k4 = 0, uint8_t k5 = 0, uint8_t k6 = 0) {

    if (!gConnected) return;

    if (gProtocolMode == 0x00) { // Boot
        if (gSubBootIn) {
            sendKeyReportRaw(gBootIn, "Boot", mods, k1, k2, k3, k4, k5, k6);
            return;
        }
    }
    else { // Report
        if (gSubReport) {
            sendKeyReportRaw(gInputReport, "Report", mods, k1, k2, k3, k4, k5, k6);
            return;
        }
    }

    // Fallback an beide, wenn (noch) nicht subscribed
    sendKeyReportRaw(gBootIn, "Boot*", mods, k1, k2, k3, k4, k5, k6);
    sendKeyReportRaw(gInputReport, "Report*", mods, k1, k2, k3, k4, k5, k6);
}

static inline void pressAndRelease(uint8_t mods, uint8_t key) {
    sendKeyReport(mods, key);
    delay(8);
    sendKeyReport(0);
    delay(5);
}

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

// ---------- DE-Mapping ----------
static bool cpToHid_DE(uint32_t cp, uint8_t& mods, uint8_t& key) {
    mods = 0;
    if (cp == '\n' || cp == '\r') { key = HID_KEY_ENTER; return true; }
    if (cp == '\t') { key = HID_KEY_TAB; return true; }
    if (cp == ' ') { key = HID_KEY_SPACEBAR; return true; }

    if (cp >= '1' && cp <= '9') { key = (uint8_t)(HID_KEY_1 + (cp - '1')); return true; }
    if (cp == '0') { key = HID_KEY_0; return true; }

    if (cp >= 'a' && cp <= 'z') {
        if (cp == 'z') { key = HID_KEY_Y; return true; }
        if (cp == 'y') { key = HID_KEY_Z; return true; }
        key = (uint8_t)(HID_KEY_A + (cp - 'a')); return true;
    }
    if (cp >= 'A' && cp <= 'Z') {
        if (cp == 'Z') { key = HID_KEY_Y; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; }
        if (cp == 'Y') { key = HID_KEY_Z; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; }
        key = (uint8_t)(HID_KEY_A + (cp - 'A')); mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
    }

    switch (cp) {
    case 0x00E4: key = HID_KEY_APOSTROPHE; return true;                               // ä
    case 0x00C4: key = HID_KEY_APOSTROPHE; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; // Ä
    case 0x00F6: key = HID_KEY_SEMICOLON;  return true;                               // ö
    case 0x00D6: key = HID_KEY_SEMICOLON;  mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; // Ö
    case 0x00FC: key = HID_KEY_LEFT_BRACKET; return true;                             // ü
    case 0x00DC: key = HID_KEY_LEFT_BRACKET; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true; // Ü
    case 0x00DF: key = HID_KEY_MINUS; return true;                                    // ß
    case 0x20AC: mods = KEYBOARD_MODIFIER_RIGHTALT; key = HID_KEY_E; return true;     // €
    }

    switch (cp) {
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

    case '/':  key = HID_KEY_7;  mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '\\': key = HID_KEY_MINUS; mods = KEYBOARD_MODIFIER_RIGHTALT; return true;
    case '#':  key = HID_KEY_BACKSLASH; return true;
    case '"':  key = HID_KEY_2; mods = KEYBOARD_MODIFIER_LEFTSHIFT; return true;
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

static inline void typeCodepoint(uint32_t cp) {
    uint8_t mods, key;
    if (!cpToHid_DE(cp, mods, key)) {
        if (cp <= 0x7F) Serial.printf("[HID] Unmapped ASCII: 0x%02X '%c'\n", (unsigned)cp, (char)cp);
        else            Serial.printf("[HID] Unmapped U+%04lX\n", (unsigned long)cp);
        return;
    }
    pressAndRelease(mods, key);
}

// ---------- Setup ----------
void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("\n[BOOT] ESP32 BLE-HID Keyboard (DE) – Boot-Protocol-First");

    NimBLEDevice::init("ESP32 DE Keyboard");
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);   // Just Works
    NimBLEDevice::setSecurityAuth(true, false, true);            // bonding, no MITM, LESC
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);                      // etwas mehr Sendeleistung
    // NimBLEDevice::setMTU(517); // optional

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* hid = server->createService((uint16_t)UUID_HID_SERVICE);

    // Protocol Mode (Default = Boot)
    NimBLECharacteristic* chProtocol = hid->createCharacteristic(
        (uint16_t)UUID_PROTOCOL_MODE,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
    );
    chProtocol->setCallbacks(&gProtoCb);
    chProtocol->setValue(&gProtocolMode, 1);

    // HID Information
    NimBLECharacteristic* chHidInfo = hid->createCharacteristic(
        (uint16_t)UUID_HID_INFORMATION,
        NIMBLE_PROPERTY::READ
    );
    uint8_t hidInfo[4] = { 0x11, 0x01, 0x00, 0x02 };
    chHidInfo->setValue(hidInfo, sizeof(hidInfo));

    // HID Control Point
    NimBLECharacteristic* chCtrlPt = hid->createCharacteristic(
        (uint16_t)UUID_HID_CONTROL_POINT,
        NIMBLE_PROPERTY::WRITE_NR
    );
    uint8_t ctl = 0x00; chCtrlPt->setValue(&ctl, 1);

    // Report Map
    NimBLECharacteristic* chReportMap = hid->createCharacteristic(
        (uint16_t)UUID_REPORT_MAP,
        NIMBLE_PROPERTY::READ
    );
    chReportMap->setValue((uint8_t*)KEYBOARD_REPORTMAP, sizeof(KEYBOARD_REPORTMAP));

    // Input Report (ID=1)
    gInputReport = hid->createCharacteristic(
        (uint16_t)UUID_REPORT,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    {
        NimBLEDescriptor* repRef = gInputReport->createDescriptor(
            (uint16_t)UUID_RPT_REF_DESC, NIMBLE_PROPERTY::READ, 2
        );
        uint8_t repRefVal[2] = { 0x01, 0x01 }; // [ReportID=1, Input]
        repRef->setValue(repRefVal, sizeof(repRefVal));
    }
    gInputReport->setCallbacks(&gSubCb); // subscribe logging

    // Boot Keyboard Input/Output
    gBootIn = hid->createCharacteristic(
        (uint16_t)UUID_BOOT_KB_INPUT,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    gBootIn->setCallbacks(&gSubCb); // subscribe logging

    hid->createCharacteristic(
        (uint16_t)UUID_BOOT_KB_OUTPUT,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );

    hid->start();

    // Optional: Device Information Service
    NimBLEService* dis = server->createService((uint16_t)0x180A);
    NimBLECharacteristic* cMan = dis->createCharacteristic((uint16_t)0x2A29, NIMBLE_PROPERTY::READ);
    cMan->setValue("TestCo");
    dis->start();

    // Advertising
    gAdv = NimBLEDevice::getAdvertising();
    gAdv->addServiceUUID((uint16_t)UUID_HID_SERVICE);
    gAdv->setAppearance(961); // HID Keyboard
    // gAdv->setScanResponse(true); // <-- in deiner NimBLE-Version nicht vorhanden
    gAdv->start();

    Serial.println("[BLE] Advertising als HID Keyboard. Bei Service-Änderungen Bond löschen & neu koppeln.");
    Serial.println("[INFO] Im seriellen Monitor tippen; Enter = Zeilenumbruch. (UTF-8)");
}

// ---------- Loop ----------
void loop() {
    // 1) Serielle Eingabe -> tippen
    while (Serial.available()) {
        uint8_t b = (uint8_t)Serial.read();
        uint32_t cp;
        if (gUtf.feed(b, cp)) {
            if (cp == '\r') continue; // nur LF
            if (!gConnected) {
                if (cp <= 0x7F) Serial.printf("[WARN] Nicht verbunden: '%c' (0x%02X)\n", (char)cp, (unsigned)cp);
                else            Serial.printf("[WARN] Nicht verbunden: U+%04lX\n", (unsigned long)cp);
                continue;
            }
            typeCodepoint(cp);
        }
    }

#ifdef AUTOTIPPER
    // 2) Auto-Tipper: alle 1000 ms (außerhalb der while-Schleife)
    if (gConnected && millis() - gLastAuto >= 1000) {
        gLastAuto = millis();
        uint8_t b = (uint8_t)gDemo[gDemoIdx++];
        if (b == 0) { gDemoIdx = 0; b = (uint8_t)gDemo[gDemoIdx++]; }
        uint32_t cp;
        static Utf8Decoder autoDec; // eigener Decoder
        if (autoDec.feed(b, cp)) {
            if (cp != '\r') typeCodepoint(cp);
        }
    }
#endif

    delay(2);
}
