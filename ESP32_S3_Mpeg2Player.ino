
// 再利用可搬性を全く考慮していない汚いソースコードです。
// This is messy source code that does not take reuse or portability into consideration at all.

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

#include "ESP32_S3_LED.h"

#include "LuniVGA_VGA.h"

//                   r,r,r,r,r,  g,g, g, g, g, g,   b, b, b, b, b,   h,v
static const PinConfig pins(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 1, 2);

//3 bit version (no resistor ladder)
//static const PinConfig pins(-1, -1, -1, -1, 8, -1, -1, -1, -1, -1, 14, -1, -1, -1, -1, 21, 1, 2);

static VGA vga;

static const Mode VGAMode = Mode::MODE_320x240x60;
static const int VGABits = 16;

static void VRAMFlush(int y) { vga.dmaBuffer->flush(vga.backBuffer, y); }

#include "Console.h"

// 黒:GND, 赤:3.3V
static const u8 SD_CS = 38; // 黄
static const u8 SD_SCK = 39; // 白
static const u8 SD_MISO = 40; // 緑 DO
static const u8 SD_MOSI = 41; // 青 DI

#define MpegFilename "/20240607_sm8347250.mpg.demux"
static const int MpegFileSize = 23105200;
static const int MpegInfoWidth = 320;
static const int MpegInfoHeight = 240;
static const int MpegInfoFps = 30;

#include "Raw.h"

#include "libmpeg2.h"

#define AppTitle "ESP32_S3_Mpeg2Player"

static void SerialPortInit() {
	Serial.begin(115200);
	Serial.print("Boot ");
	Serial.println(AppTitle);
}

static void DrawColorBar() {
	for (int y = 0; y < VGAMode.vRes; y++) {
		for (int x = 0; x < VGAMode.hRes; x++) {
			vga.dotdit(x, y, x, y, 255 - x);
		}
	}

	for (int y = 0; y < 30; y++) {
		for (int x = 0; x < 256; x++) {
			vga.dotdit(x, y, x, 0, 0);
			vga.dotdit(x, y + 30, 0, x, 0);
			vga.dotdit(x, y + 60, 0, 0, x);
		}
	}

	vga.show();
}

static void ClearVRAM() {
	for (int y = 0; y < VGAMode.vRes; y++) {
		for (int x = 0; x < VGAMode.hRes; x++) {
			vga.dot(x, y, 0, 0, 0);
		}
	}

	vga.show();
}

#include "ShowChipInfos.h"

void setup() {
	//delay(3000);

	SerialPortInit();

	ESP32_S3_LED_Init();

	if (!vga.init(pins, VGAMode, VGABits)) {
		Serial.println("VGA init error.");
		while (1) { delay(1); }
	}
	vga.start();

	DrawColorBar();
	delay(3000);

	ConsoleSetFontHeight(98);
	ConsoleWriteLine("Boot " AppTitle);
	ConsoleWriteLine("VGA output: " + String(VGAMode.hRes) + "x" + String(VGAMode.vRes) + "pixels,");
	ConsoleWriteLine("            60Hz, " + String(vga.bits) + "bpp.");

	ConsoleSetFontHeight(68);

	ConsoleWriteLine("-----------------------------------------------");

	ShowChipInfos();

	ESP32_S3_LED_SetRGB(0, 0x40, 0);
	ConsoleWriteLine("Wait 3secs."); delay(3000);

	Raw_Init();

	if (!libmpeg2_Init()) {
		Serial.println("libmpeg2_Init: Failed.");
		while (1) { delay(1); }
	}

	ESP32_S3_LED_SetRGB(0x40, 0, 0);
	ConsoleWriteLine("Wait 1secs."); delay(1000);

	Raw_ReadTest();

	ESP32_S3_LED_SetRGB(0, 0, 0x40);
	ConsoleWriteLine("Wait 1secs."); delay(1000);

	ESP32_S3_LED_SetBlack();

	ClearVRAM();
}

static int BGA_FrameIndex = 0;

#include "LEDTimeLine.h"

void loop() {
	static u32 StartMillis = 0;
	if (StartMillis == 0) { StartMillis = millis(); }

	const int NextMillis = BGA_FrameIndex * 1000 / MpegInfoFps;
	const int DelayMillis = (millis() - StartMillis) - NextMillis;

	if (false) {
		int LED = DelayMillis * 0x100 / 1000; // 1秒遅れると最大輝度
		if (0x40 < LED) { LED = 0x40; }
		ESP32_S3_LED_SetRGB(LED, LED, LED);
	}

	if ((1000 / MpegInfoFps) <= DelayMillis) {
		//Serial.println(String(BGA_FrameIndex) + " Delay: " + String(DelayMillis) + "ms.");
		libmpeg2_SetVideoEnabled(false);
	}
	else {
		libmpeg2_SetVideoEnabled(true);
	}

	if (0 <= DelayMillis) {
		{
			static bool f = false;
			static u8 rtbl[0x100], gtbl[0x100], btbl[0x100];
			if (!f) {
				f = true;
				for (u32 a = 0; a < 0x100; a++) {
					const u32 v = (a * a * a * a) / (0xff * 0xff * 0xff);
					rtbl[a] = v * (1 - 0.299) * 0.25;
					gtbl[a] = v * (1 - 0.587) * 0.25;
					btbl[a] = v * (1 - 0.114) * 0.25;
				}
			}

			const u8* pRGB = &LEDTimeLine[BGA_FrameIndex * 3];
			const u8 R = pRGB[0];
			const u8 G = pRGB[1];
			const u8 B = pRGB[2];
			ESP32_S3_LED_SetRGBDirect(rtbl[R], gtbl[G], btbl[B]);
		}

		// Frame decode
		if (!libmpeg2_DrawFrame()) {
			libmpeg2_Close();

			ConsoleSetFontHeight(16);
			const int fh = 16;
			ConsoleSetLine(VGAMode.vRes - ((fh * 2) + (fh * 2)));
			ConsoleWriteLine("End of video.");
			ConsoleSetLine(VGAMode.vRes - ((fh * 2) + (fh * 0)));
			ConsoleWriteLine("Thank you for watching the stream!");

			while (1) { delay(1); }
		}
		BGA_FrameIndex++;
		return;
	}
	else {
		if (libmpeg2_PreloadFile(false)) { return; }
	}

	delay(1);
}
