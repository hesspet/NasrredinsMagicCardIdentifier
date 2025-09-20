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

# define SERIAL3_RX_PIN 26
# define SERIAL3_TX_PIN 25

/* When the number after #if set as 1, it will be switch to SPI Mode*/
#include <PN532_HSU.h>
#include <PN532.h>

PN532_HSU pn532hsu(Serial2); // angepaßt für NMCI auf Serial2
PN532 nfc(pn532hsu);

void setup(void) {

	Serial.begin(115200);
	Serial.println("Hello from SimpleRead_iso14443a_uid.ini V1");

	// Serielle Schnittstelle auf die gewünschten Ports umlegen
	Serial2.begin(115200, SERIAL_8N1, SERIAL3_RX_PIN, SERIAL3_TX_PIN); 

	nfc.begin();

	// ein wenig Info ausgeben über das Board
	uint32_t versiondata = nfc.getFirmwareVersion();
	if (!versiondata) {
		Serial.print("Didn't find PN53x board");
		while (1); // halt // TODO: In diesem Fall sollte man vielleicht etwas auf das T-Display schreiben
	}

	// Alles OK, schreiben wir ein paar Infos raus
	Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
	Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
	Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

	// Set the max number of retry attempts to read from a card
	// This prevents us from waiting forever for a card, which is
	// the default behaviour of the PN532.
	nfc.setPassiveActivationRetries(0xFF); // TODO: Das ist aber nur zum Testen sinnvoll? Klären

	// configure board to read RFID tags
	nfc.SAMConfig();

	Serial.println("Waiting for an ISO14443A card");
}

void loop(void) {

	boolean success;
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
	uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

	// Wait for an ISO14443A type cards (Mifare, NTAG213 etc.).  When one is found
	// 'uid' will be populated with the UID, and uidLength will indicate
	// if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
	
	success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

	if (success) {
		Serial.println("Found a card!");
		Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
		Serial.print("UID Value: ");

		// NTAG213 7-Byte!

		for (uint8_t i = 0; i < uidLength; i++)
		{
			Serial.print(" 0x"); Serial.print(uid[i], HEX);
		}
		Serial.println("");

		// Wait 1 second before continuing
		delay(1000);
	}
	else
	{
		// PN532 probably timed out waiting for a card
		Serial.println("Timed out waiting for a card");
	}



}
