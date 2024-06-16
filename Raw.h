#pragma once

#include <SD.h>
#include "isd_diskio.h"

static u8 pdrv = 0xff; // SD card handler

static void Raw_ReadSectors(void* pbuf, const int TopSector, const int SectorsCount) {
	if (!sdReadSectors(pdrv, (char*)pbuf, TopSector, SectorsCount)) {
		ConsoleWriteLine("Raw_ReadSectors: Failed.");
		while (true) { delay(1); }
	}
}

static void Raw_Init() {
	ConsoleWriteLine("Open SD card.");
	SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1);
	pdrv = sdcard_init(SD_CS, &SPI, 40 * 1000 * 1000); // 規格上の最大周波数は24MHzらしい。40MHzで通信可能かはSDカードとの相性による
	ConsoleWriteLine("pdrv: $" + String(pdrv, HEX));
	if (pdrv == 0xff) {
		Serial.println("Failed.");
		while (true) { delay(1); }
	}

	ConsoleWriteLine("ff_sd_initialize.");
	u8 sdinitres = ff_sd_initialize(pdrv);
	if (sdinitres != 0x00) {
		Serial.println("Failed. $" + String(sdinitres, HEX));
		while (true) { delay(1); }
	}
	ConsoleWriteLine(String("Filename: ") + MpegFilename);
}

static void Raw_ReadTest() {
	ConsoleWriteLine("Read test. (Sector 1)");
	u8 buf[512];
	if (!sdReadSectors(pdrv, (char*)buf, 1, 1)) {
		Serial.println("Failed.");
		while (true) { delay(1); }
	}
	ConsoleSetFontHeight(68);
	for (int y = 0; y < 16; y++) {
		String Line = "$" + String(y * 16, HEX) + " ";
		for (int x = 0; x < 16; x++) {
			Line += String(buf[y * 16 + x], HEX) + " ";
		}
		ConsoleWriteLine(Line);
	}
}