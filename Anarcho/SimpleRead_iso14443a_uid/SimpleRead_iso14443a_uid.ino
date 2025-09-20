/**************************************************************************/
/*!
	Integrationstest für T-Display auf Basis des Beispiels
	in der Bibliothek PN532

	Dient einfach dem Anschalttest

	- Serial2 wird auf pin 26/26 gelegt
	- Keine weitere Konfiguration an dem Board notwendig
	- VDD -> 3,3V ! und wird vom T-Display abgegriffen

	This example will attempt to connect to an ISO14443A
	card or tag and retrieve some basic information about it
	that can be used to determine what type of card it is.

	Note that you need the baud rate to be 115200 because we need to print
	out the data and read from the card at the same time!

	To enable debug message, define DEBUG in PN532/PN532_debug.h

*/
/**************************************************************************/

#include <PN532_HSU.h>
#include <PN532.h>

#include <ctype.h>
#include <string.h>

#define PN532_HSU_PORT Serial2
constexpr auto PN532_HSU_BAUDRATE = 115200;
constexpr auto PN532_HSU_RX_PIN = 26;
constexpr auto PN532_HSU_TX_PIN = 25;

static PN532_HSU pn532hsu(PN532_HSU_PORT);
static PN532 nfc(pn532hsu);

namespace 
{

	constexpr uint8_t kUidBufferSize = 7;
	constexpr uint8_t kFirstUserPage = 4;
	constexpr uint8_t kPagesPerBlock = 4;
	constexpr uint8_t kBytesPerPage = 4;
	constexpr size_t kBlockSize = kPagesPerBlock * kBytesPerPage;

	bool readFirstDataBlock(uint8_t* buffer, size_t length) 
	{
		if (buffer == nullptr || length < kBlockSize) 
		{
			return false;
		}

		for (uint8_t page = 0; page < kPagesPerBlock; ++page) 
		{
			if (!nfc.mifareultralight_ReadPage(kFirstUserPage + page,
				buffer + page * kBytesPerPage)) 
			{
				return false;
			}
		}
		return true;
	}

	void printBlockAsString(const uint8_t* data, size_t length) 
	{
		char text[kBlockSize + 1]{};
		size_t textLength = 0;

		for (; textLength < length && textLength < kBlockSize; ++textLength) 
		{
			char c = static_cast<char>(data[textLength]);
			if (c == '\0') 
			{
				break;
			}

			if (isprint(static_cast<unsigned char>(c)) || c == '\r' || c == '\n') 
			{
				text[textLength] = c;
			}
			else 
			{
				text[textLength] = '?';
			}
		}
		text[textLength] = '\0';

		if (textLength == 0) 
		{
			Serial.println(F("First NTAG213 data block is empty"));
			return;
		}

		Serial.print(F("First NTAG213 data block: "));
		Serial.println(text);
	}

	bool isSameUid(const uint8_t* lhs, const uint8_t* rhs, uint8_t length) 
	{
		return lhs && rhs && (memcmp(lhs, rhs, length) == 0);
	}
}  // namespace

void setup() {

	Serial.begin(115200);
	while (!Serial) 
	{
		delay(10);
	}

	PN532_HSU_PORT.begin(PN532_HSU_BAUDRATE, SERIAL_8N1, PN532_HSU_RX_PIN, PN532_HSU_TX_PIN);

	nfc.begin();

	uint32_t versiondata = nfc.getFirmwareVersion();
	if (!versiondata) 
	{
		Serial.println(F("PN532 not found"));

		while (true) 
		{
			delay(1000);
		}
	}

	Serial.print(F("Found chip PN5"));
	Serial.println((versiondata >> 24) & 0xFF, HEX);
	Serial.print(F("Firmware ver. "));
	Serial.print((versiondata >> 16) & 0xFF, DEC);
	Serial.print('.');
	Serial.println((versiondata >> 8) & 0xFF, DEC);

	nfc.SAMConfig();
	nfc.setPassiveActivationRetries(0xFF);

	Serial.println(F("Waiting for an ISO14443A Card ..."));
}

void loop() 
{
	static uint8_t lastUid[kUidBufferSize];
	static uint8_t lastUidLength = 0;
	static bool tagPresent = false;

	uint8_t uid[kUidBufferSize];
	uint8_t uidLength = 0;

	if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) 
	{
		bool newTagDetected = !tagPresent || (uidLength != lastUidLength) ||
			!isSameUid(uid, lastUid, uidLength);

		if (newTagDetected) 
		{
			tagPresent = true;
			lastUidLength = uidLength;
			memcpy(lastUid, uid, uidLength);

			Serial.print(F("Tag detected. UID length: "));
			Serial.println(uidLength);
			Serial.print(F("UID:"));
			nfc.PrintHex(uid, uidLength);

			if (uidLength == 7) 
			{
				uint8_t block[kBlockSize];
				if (readFirstDataBlock(block, sizeof(block))) 
				{
					Serial.println(F("Read first NTAG213 data block"));
					printBlockAsString(block, sizeof(block));
				}
				else 
				{
					Serial.println(F("Failed to read first NTAG213 data block"));
				}
			}
			else 
			{
				Serial.println(F("Tag is not an NTAG213 (expected 7-byte UID)"));
			}
		}
	}
	else if (tagPresent) 
	{
		tagPresent = false;
		lastUidLength = 0;
		Serial.println(F("Tag removed"));
	}

	delay(250);
}
