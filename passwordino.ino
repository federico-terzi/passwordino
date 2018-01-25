#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Bounce2.h>

// Software version
#define PASSWORDINO_VERSION "0.1"

// Buttons
int btnPins[] = {4,5,6,7,8,9};
int btnValues[] = {1,2,3,4,5,6};
#define NUM_BUTTONS 6
Bounce dbtn[NUM_BUTTONS];

// Display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// Typing mode ( which method to use to inject keys )
#define WIN_MODE 1  // More precise, but only works on windows.
					// Inject keys by typing ALT + NUMPAD NUMBER
					// to directly specify the ascii character.
#define GEN_MODE 2   // Type letters using the standard USA keyboard
					// layout. The problem is that if you use a non USA
					// keyboard, all the symbols may appear incorrectly.
int typeMode = GEN_MODE;

//The setup function is called once at startup of the sketch
void setup() {
	Serial.begin(9600);

	display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x64)

	// Clear the buffer.
	display.clearDisplay();

	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.println("*********************");
	display.println("**** Passwordino ****");
	display.  print("****    v ");
	display.print(PASSWORDINO_VERSION);
	display.println("    ****");
	display.println("*********************");
	display.display();
	delay(1);

	// Set up the buttons
	for (int i = 0; i< NUM_BUTTONS; i++) {
		pinMode(btnPins[i], INPUT_PULLUP);

		// Set up the debauncer
		dbtn[i].attach(btnPins[i]);
		dbtn[i].interval(5);
	}
}

// The loop function is called in an endless loop
void loop() {
	// Update the debauncer
	for (int i = 0; i< NUM_BUTTONS; i++) {
		dbtn[i].update();
	}

	// Check if a button was pressed
	for (int i = 0; i< NUM_BUTTONS; i++) {
		if (dbtn[i].fell()) {
			handleBtnPress(btnValues[i]);
		}
	}

	renderDisplay();
}

void handleBtnPress(int value) {
	Serial.println(value);
	if (value==4) {
		typeMode = WIN_MODE;
	}
}

void renderDisplay() {
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);

	// Print the topbar
	display.print("MODE: ");
	switch(typeMode) {
		case WIN_MODE:
			display.print("WIN");
			break;
		case GEN_MODE:
			display.print("GEN");
			break;
	}
	display.println();
	display.drawLine(0, 8, display.width(), 8, WHITE);


	display.display();
}


void progressbar(void) {
	uint8_t color = 1;
	for (int16_t i = 0; i < display.width(); i += 1) {
		display.fillRect(0, 25, i, 5, color % 2);
		display.display();
		delay(1);
		color++;
	}
}
