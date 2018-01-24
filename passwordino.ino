#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PASSWORDINO_VERSION "0.1"

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

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
}

// The loop function is called in an endless loop
void loop() {
//Add your repeated code here
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
