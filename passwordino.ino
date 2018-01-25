#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Bounce2.h>
#include <EEPROM.h>
#include <AESLib.h>
#include <Keyboard.h>

// Software version
#define PASSWORDINO_VERSION "0.1"

// Serial
#define BAUD_RATE 9600

#define PASSWORD_NAME_MAX_LEN 12
#define PASSWORD_MAX_LEN 32  // Multiple of 16
#define MAX_KEY_LEN 8
#define MAX_PASSWORD_NUM 10
#define MAGIC_NUMBER 123

// Buttons
uint8_t btnPins[] = { 4, 5, 6, 7, 8, 9 };
uint8_t btnValues[] = { 1, 2, 3, 4, 5, 6 };
#define NUM_BUTTONS 6
#define DEBUG_BTN_PIN 9
Bounce dbtn[NUM_BUTTONS];

// Display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define CHARS_PER_LINE 21

// PASSWORD SELECT MODE
#define OK_BTN 1
#define DOWN_BTN 2
#define UP_BTN 3
#define BACK_BTN 6

// MODES
#define PASSWORD_SELECT_MODE 1
#define PASSWORD_TYPING_MODE 2
#define SERIAL_MODE 3

// Current mode of the device, starts in the main menu
uint8_t mode = PASSWORD_SELECT_MODE;

// Typing mode ( which method to use to inject keys )
#define WIN_TYPE_MODE 0  	// More precise, but only works on windows.
// Inject keys by typing ALT + NUMPAD NUMBER
// to directly specify the ascii character.
#define GEN_TYPE_MODE 1  	// Type letters using the standard USA keyboard
// layout. The problem is that if you use a non USA
// keyboard, all the symbols may appear incorrectly.
uint8_t typeMode = GEN_TYPE_MODE;

// Current row in the list
uint8_t currentRow = 0;

typedef struct {
	uint8_t encrypted;
	char name[PASSWORD_NAME_MAX_LEN + 1]; // With \0 terminator included
	char pass[PASSWORD_MAX_LEN + 1];
} PasswordEntry;

// Array containing the password entries
PasswordEntry pwArray[MAX_PASSWORD_NUM];
int pwLen = 0;
uint8_t validKeyChars[] = { 1, 2, 3, 4, 5 };

//The setup function is called once at startup of the sketch
void setup() {
	Serial.begin(BAUD_RATE);

	// Set up the buttons
	for (int i = 0; i < NUM_BUTTONS; i++) {
		pinMode(btnPins[i], INPUT_PULLUP);

		// Set up the debauncer
		dbtn[i].attach(btnPins[i]);
		dbtn[i].interval(5);
	}

	// Initialize the display at 0x3C
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	// Clear the buffer.
	display.clearDisplay();

	// Print the logo screen
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.println("*********************");
	display.println("**** Passwordino ****");
	display.print("****    v ");
	display.print(PASSWORDINO_VERSION);
	display.println("    ****");
	display.println("*********************");
	display.display();
	delay(1);

	// If debug button is pressed, wait for the serial connection
	if (digitalRead(DEBUG_BTN_PIN) == LOW) {
		display.clearDisplay();
		display.setTextSize(1);
		display.setTextColor(WHITE);
		display.setCursor(0, 0);
		display.println("Waiting for serial...");
		display.print("BAUD: ");
		display.println(BAUD_RATE);
		display.display();
		delay(10);

		while (!Serial) {
		}

		mode = SERIAL_MODE;
		Serial.setTimeout(1000000);
	}

	Serial.print("Passwordino v");
	Serial.println(PASSWORDINO_VERSION);
	Serial.println("Loading EEPROM...");

	loadPasswordsFromEEPROM();

	Serial.println("Done");
}

// The loop function is called in an endless loop
void loop() {
	// Update the debauncer
	for (int i = 0; i < NUM_BUTTONS; i++) {
		dbtn[i].update();
	}

	// Check if a button was pressed
	for (int i = 0; i < NUM_BUTTONS; i++) {
		if (dbtn[i].fell()) {
			handleBtnPress(btnValues[i]);
		}
	}

	if (mode == SERIAL_MODE) {
		handleSerial();
	}

	renderDisplay();

	delay(1);
}

void renderDisplay() {
	switch (mode) {
	case PASSWORD_SELECT_MODE:
		renderPasswordList();
		break;
	case SERIAL_MODE:
		renderSerialMode();
		break;
	}
}

void renderPasswordList() {
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);

	if (pwLen > 0) {
		int start, finish;
		if (currentRow < 4) {
			start = 0;
			finish = (pwLen < 4) ? pwLen : 4;
		} else if (currentRow > (pwLen - 4)) {
			start = pwLen - 4;
			finish = pwLen;
		} else {
			start = currentRow - 3;
			finish = currentRow + 1;
		}

		for (int i = start; i < finish; i++) {
			if (currentRow == i) {
				display.setTextColor(BLACK, WHITE);
			} else {
				display.setTextColor(WHITE);
			}
			display.print('[');
			if (!pwArray[i].encrypted) {
				display.print(' ');
			} else {
				display.print('*');
			}
			display.print(']');
			display.print(' ');
			display.print(pwArray[i].name);
			int nameLen = strlen(pwArray[i].name);
			for (int j = (nameLen + 4); j < CHARS_PER_LINE; j++) {
				display.print(' ');
			}
			display.println();

		}
	} else {
		display.println("No passwords");
	}

	display.display();
}

void renderSerialMode() {
	display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);

	display.println("SERIAL MODE");
	display.setTextSize(1);
	display.print("BAUD: ");
	display.println(BAUD_RATE);

	display.display();
}

void loadPasswordsFromEEPROM() {
	int eeAddress = 0;
	uint8_t statusCode = 0;
	PasswordEntry current;
	// Reset the array
	memset(pwArray, 0, sizeof(PasswordEntry) * MAX_PASSWORD_NUM);
	pwLen = 0;
	while (eeAddress < EEPROM.length()) {
		EEPROM.get(eeAddress, statusCode);
		eeAddress += sizeof(statusCode);
		if (statusCode == 123) { // Magic number
			EEPROM.get(eeAddress, current);
			memcpy(&pwArray[pwLen], &current, sizeof(PasswordEntry));
			pwLen++;
			eeAddress += sizeof(current);
		} else {
			break;
		}
	}
}

int findFreeIndexInEEPROM() {
	int eeAddress = 0;
	uint8_t statusCode = 0;

	while (eeAddress < EEPROM.length()) {
		EEPROM.get(eeAddress, statusCode);
		if (statusCode == MAGIC_NUMBER) { // Magic number
			eeAddress += sizeof(statusCode);
			eeAddress += sizeof(PasswordEntry);
		} else {
			return eeAddress;
		}
	}

	return -1;
}

int writePasswordToEEPROM(char *name, char *pass, uint8_t isEncrypted,
		uint8_t key[], uint8_t keyLen) {
	int freeIndex = findFreeIndexInEEPROM();

	if (freeIndex < 0) {
		return 0;
	}

	uint8_t magicNumber = MAGIC_NUMBER;
	PasswordEntry current;
	current.encrypted = isEncrypted;
	strcpy(current.name, name);

	if (!isEncrypted) {
		strcpy(current.pass, pass);
	} else {
		char encryptedPass[PASSWORD_MAX_LEN];
		strcpy(encryptedPass, "OK");
		strcat(encryptedPass, pass);

		aes128_enc_single(key, encryptedPass);
		aes128_enc_single(key, &encryptedPass[16]);
		strcpy(current.pass, encryptedPass);
	}

	EEPROM.put(freeIndex, magicNumber);
	freeIndex += sizeof(magicNumber);
	EEPROM.put(freeIndex, current);

	return 1;
}

void clearEEPROM() {
	for (int i = 0; i < EEPROM.length(); i++) {
		EEPROM.write(i, 0);
	}
}

void handleBtnPress(uint8_t value) {
	Serial.println(value);

	switch (mode) {
	case PASSWORD_SELECT_MODE:
		switch (value) {
		case DOWN_BTN:
			if (currentRow < (pwLen - 1)) {
				currentRow++;
			}
			break;
		case UP_BTN:
			if (currentRow > 0) {
				currentRow--;
			}
			break;
		case OK_BTN:
			if (pwLen > 0) {
				handlePasswordSelect();
			}
			break;
		}
		break;
	}

}

void handlePasswordSelect() {
	PasswordEntry current = pwArray[currentRow];

}

void handleSerial() {
	String cmd = Serial.readStringUntil('\n');

	if (cmd.equals("add")) {
		Serial.println("Type the name: ");
		String name = Serial.readStringUntil('\n');
		if (name.length() > PASSWORD_NAME_MAX_LEN) {
			Serial.println("Too long!");
			return;
		}
		char cname[PASSWORD_NAME_MAX_LEN];
		name.toCharArray(cname, PASSWORD_NAME_MAX_LEN);

		Serial.println("Type the password: ");
		String pass = Serial.readStringUntil('\n');
		if (pass.length() > PASSWORD_MAX_LEN) {
			Serial.println("Too long!");
			return;
		}
		char cpass[PASSWORD_MAX_LEN];
		pass.toCharArray(cpass, PASSWORD_MAX_LEN);

		Serial.println(
				"Encrypt Code (type enter if you don't want to encrypt it):");
		String code = Serial.readStringUntil('\n');

		if (code.length() == 0) {  // Not encrypted
			if (!writePasswordToEEPROM(cname, cpass, 0, NULL, 0)) {
				Serial.println("Can't save password!");
				return;
			}
		} else if (code.length() > MAX_KEY_LEN) {
			Serial.println("Key too long!");
			return;
		} else {  // Encrypted
			uint8_t keys[MAX_KEY_LEN];
			for (int i = 0; i < code.length(); i++) {
				keys[i] = atoi(code.charAt(i));
			}
			if (!writePasswordToEEPROM(cname, cpass, 1, keys, code.length())) {
				Serial.println("Can't save password!");
				return;
			}
		}
		Serial.println("Done!");
	} else if (cmd.equals("list")) {
		loadPasswordsFromEEPROM();
		if (pwLen == 0) {
			Serial.println("No entries");
		} else {
			for (int i = 0; i < pwLen; i++) {
				Serial.print(pwArray[i].name);
				Serial.print('\t');
				Serial.println(pwArray[i].encrypted);
			}
		}
	} else if (cmd.equals("clear")) {
		clearEEPROM();
		Serial.println("Cleared!");
	} else {
		Serial.println("Command not recognized!");
	}
}

void switchTypeMode() {
	switch (typeMode) {
	case WIN_TYPE_MODE:
		typeMode = GEN_TYPE_MODE;
		break;
	case GEN_TYPE_MODE:
		typeMode = WIN_TYPE_MODE;
		break;
	}
}

void progressbar() {
	uint8_t color = 1;
	for (int16_t i = 0; i < display.width(); i += 1) {
		display.fillRect(0, 25, i, 5, color % 2);
		display.display();
		delay(1);
		color++;
	}
}
