
#include "Adafruit_NeoPixel.h"

#define ESP32_S3_LED_LED_PIN (48) // Define the pin where the built-in RGB LED is connected

#define ESP32_S3_LED_NUM_LEDS (1) // Define the number of LEDs in the strip (usually 1 for built-in LED)

static Adafruit_NeoPixel ESP32_S3_LED_Strip = Adafruit_NeoPixel(ESP32_S3_LED_NUM_LEDS, ESP32_S3_LED_LED_PIN, NEO_GRB + NEO_KHZ800); // Create an instance of the Adafruit_NeoPixel class

static void ESP32_S3_LED_SetRGB(int r, int g, int b) {
	if (r < 0x00) { r = 0x00; }
	if (0xff < r) { r = 0xff; }
	if (g < 0x00) { g = 0x00; }
	if (0xff < g) { g = 0xff; }
	if (b < 0x00) { b = 0x00; }
	if (0xff < b) { b = 0xff; }
	u32 Data = ESP32_S3_LED_Strip.Color(ESP32_S3_LED_Strip.gamma8(r), ESP32_S3_LED_Strip.gamma8(g), ESP32_S3_LED_Strip.gamma8(b));
	ESP32_S3_LED_Strip.setPixelColor(0, Data);
	ESP32_S3_LED_Strip.show();
}

static void ESP32_S3_LED_SetRGBDirect(int r, int g, int b) {
	if (r < 0x00) { r = 0x00; }
	if (0xff < r) { r = 0xff; }
	if (g < 0x00) { g = 0x00; }
	if (0xff < g) { g = 0xff; }
	if (b < 0x00) { b = 0x00; }
	if (0xff < b) { b = 0xff; }
	u32 Data = ESP32_S3_LED_Strip.Color(r, g, b);
	ESP32_S3_LED_Strip.setPixelColor(0, Data);
	ESP32_S3_LED_Strip.show();
}

static void ESP32_S3_LED_SetBlack() { ESP32_S3_LED_SetRGBDirect(0x00, 0x00, 0x00); }
static void ESP32_S3_LED_SetRed() { ESP32_S3_LED_SetRGBDirect(0xff, 0x00, 0x00); }
static void ESP32_S3_LED_SetGreen() { ESP32_S3_LED_SetRGBDirect(0x00, 0xff, 0x00); }
static void ESP32_S3_LED_SetBlue() { ESP32_S3_LED_SetRGBDirect(0x00, 0x00, 0xff); }

static void ESP32_S3_LED_Init() {
	ESP32_S3_LED_Strip.begin(); // Initialize the NeoPixel library
	ESP32_S3_LED_SetBlack();
}

