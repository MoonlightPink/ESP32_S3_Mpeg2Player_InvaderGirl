// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _ISD_DISKIO_H_
#define _ISD_DISKIO_H_

#include "Arduino.h"
#include "SPI.h"
#include "sd_defines.h"
// #include "diskio.h"

#endif /* _SD_DISKIO_H_ */

// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//#include "esp_system.h"
extern "C" {
#include "ff.h"
	//#include "diskio.h"
#if ESP_IDF_VERSION_MAJOR > 3
#include "diskio_impl.h"
#endif
	//#include "esp_vfs.h"
//#include "esp_vfs_fat.h"
	char CRC7(const char* data, int length);
	unsigned short CRC16(const char* data, int length);
}

typedef enum {
	GO_IDLE_STATE = 0,
	SEND_OP_COND = 1,
	SEND_CID = 2,
	SEND_RELATIVE_ADDR = 3,
	SEND_SWITCH_FUNC = 6,
	SEND_IF_COND = 8,
	SEND_CSD = 9,
	STOP_TRANSMISSION = 12,
	SEND_STATUS = 13,
	SET_BLOCKLEN = 16,
	READ_BLOCK_SINGLE = 17,
	READ_BLOCK_MULTIPLE = 18,
	SEND_NUM_WR_BLOCKS = 22,
	SET_WR_BLK_ERASE_COUNT = 23,
	WRITE_BLOCK_SINGLE = 24,
	WRITE_BLOCK_MULTIPLE = 25,
	APP_OP_COND = 41,
	APP_CLR_CARD_DETECT = 42,
	APP_CMD = 55,
	READ_OCR = 58,
	CRC_ON_OFF = 59
} ardu_sdcard_command_t;

typedef struct {
	uint8_t ssPin;
	SPIClass* spi;
	int frequency;
	char* base_path;
	sdcard_type_t type;
	unsigned long sectors;
	bool supports_crc;
	int status;
} ardu_sdcard_t;

static ardu_sdcard_t* s_cards[FF_VOLUMES] = { NULL };

namespace {

	struct AcquireSPI {
		ardu_sdcard_t* card;
		explicit AcquireSPI(ardu_sdcard_t* card)
			: card(card) {
			card->spi->beginTransaction(SPISettings(card->frequency, MSBFIRST, SPI_MODE0));
		}
		AcquireSPI(ardu_sdcard_t* card, int frequency)
			: card(card) {
			card->spi->beginTransaction(SPISettings(frequency, MSBFIRST, SPI_MODE0));
		}
		~AcquireSPI() {
			card->spi->endTransaction();
		}
	private:
		AcquireSPI(AcquireSPI const&);
		AcquireSPI& operator=(AcquireSPI const&);
	};

}

/*
 * SD SPI
 * */

static bool sdWait(uint8_t pdrv, int timeout) {
	char resp;
	uint32_t start = millis();

	do {
		resp = s_cards[pdrv]->spi->transfer(0xFF);
	} while (resp == 0x00 && (millis() - start) < (unsigned int)timeout);

	if (!resp) {
		log_w("Wait Failed");
	}
	return (resp > 0x00);
}

static void sdStop(uint8_t pdrv) {
	s_cards[pdrv]->spi->write(0xFD);
}

static void sdDeselectCard(uint8_t pdrv) {
	ardu_sdcard_t* card = s_cards[pdrv];
	digitalWrite(card->ssPin, HIGH);
}

static bool sdSelectCard(uint8_t pdrv) {
	ardu_sdcard_t* card = s_cards[pdrv];
	digitalWrite(card->ssPin, LOW);
	bool s = sdWait(pdrv, 500);
	if (!s) {
		log_e("Select Failed");
		digitalWrite(card->ssPin, HIGH);
		return false;
	}
	return true;
}

static char sdCommand(uint8_t pdrv, char cmd, unsigned int arg, unsigned int* resp) {
	char token;
	ardu_sdcard_t* card = s_cards[pdrv];

	for (int f = 0; f < 3; f++) {
		if (cmd == SEND_NUM_WR_BLOCKS || cmd == SET_WR_BLK_ERASE_COUNT || cmd == APP_OP_COND || cmd == APP_CLR_CARD_DETECT) {
			token = sdCommand(pdrv, APP_CMD, 0, NULL);
			sdDeselectCard(pdrv);
			if (token > 1) {
				break;
			}
			if (!sdSelectCard(pdrv)) {
				token = 0xFF;
				break;
			}
		}

		char cmdPacket[7];
		cmdPacket[0] = cmd | 0x40;
		cmdPacket[1] = arg >> 24;
		cmdPacket[2] = arg >> 16;
		cmdPacket[3] = arg >> 8;
		cmdPacket[4] = arg;
		if (card->supports_crc || cmd == GO_IDLE_STATE || cmd == SEND_IF_COND) {
			cmdPacket[5] = (CRC7(cmdPacket, 5) << 1) | 0x01;
		} else {
			cmdPacket[5] = 0x01;
		}
		cmdPacket[6] = 0xFF;

		card->spi->writeBytes((uint8_t*)cmdPacket, (cmd == STOP_TRANSMISSION) ? 7 : 6);

		for (int i = 0; i < 9; i++) {
			token = card->spi->transfer(0xFF);
			if (!(token & 0x80)) {
				break;
			}
		}

		if (token == 0xFF) {
			log_w("no token received");
			sdDeselectCard(pdrv);
			delay(100);
			sdSelectCard(pdrv);
			continue;
		} else if (token & 0x08) {
			log_w("crc error");
			sdDeselectCard(pdrv);
			delay(100);
			sdSelectCard(pdrv);
			continue;
		} else if (token > 1) {
			log_w("token error [%u] 0x%x", cmd, token);
			break;
		}

		if (cmd == SEND_STATUS && resp) {
			*resp = card->spi->transfer(0xFF);
		} else if ((cmd == SEND_IF_COND || cmd == READ_OCR) && resp) {
			*resp = card->spi->transfer32(0xFFFFFFFF);
		}

		break;
	}
	if (token == 0xFF) {
		log_e("Card Failed! cmd: 0x%02x", cmd);
		card->status = STA_NOINIT;
	}
	return token;
}

static bool sdReadBytes(uint8_t pdrv, char* buffer, int length) {
	char token;
	unsigned short crc;
	ardu_sdcard_t* card = s_cards[pdrv];

	uint32_t start = millis();
	do {
		token = card->spi->transfer(0xFF);
	} while (token == 0xFF && (millis() - start) < 500);

	if (token != 0xFE) {
		return false;
	}

	card->spi->transferBytes(NULL, (uint8_t*)buffer, length);
	crc = card->spi->transfer16(0xFFFF);

	return true; // CRCチェックをしない

	return (!card->supports_crc || crc == CRC16(buffer, length));
}

/*
 * SPI SDCARD Communication
 * */

static char sdTransaction(uint8_t pdrv, char cmd, unsigned int arg, unsigned int* resp) {
	if (!sdSelectCard(pdrv)) {
		return 0xFF;
	}
	char token = sdCommand(pdrv, cmd, arg, resp);
	sdDeselectCard(pdrv);
	return token;
}

static bool sdReadSectors(uint8_t pdrv, char* buffer, unsigned long long sector, int count) {
	AcquireSPI card_locked(s_cards[pdrv]);

	for (int f = 0; f < 3;) {
		if (!sdSelectCard(pdrv)) {
			return false;
		}

		if (!sdCommand(pdrv, READ_BLOCK_MULTIPLE, (s_cards[pdrv]->type == CARD_SDHC) ? sector : sector << 9, NULL)) {
			do {
				if (!sdReadBytes(pdrv, buffer, 512)) {
					f++;
					break;
				}

				sector++;
				buffer += 512;
				f = 0;
			} while (--count);

			if (sdCommand(pdrv, STOP_TRANSMISSION, 0, NULL)) {
				log_e("command failed");
				break;
			}

			sdDeselectCard(pdrv);
			if (count == 0) {
				return true;
			}
		} else {
			break;
		}
	}
	sdDeselectCard(pdrv);
	return false;
}

static unsigned long sdGetSectorsCount(uint8_t pdrv) {
	for (int f = 0; f < 3; f++) {
		if (!sdSelectCard(pdrv)) {
			return false;
		}

		if (!sdCommand(pdrv, SEND_CSD, 0, NULL)) {
			char csd[16];
			bool success = sdReadBytes(pdrv, csd, 16);
			sdDeselectCard(pdrv);
			if (success) {
				if ((csd[0] >> 6) == 0x01) {
					unsigned long size = (
						((unsigned long)(csd[7] & 0x3F) << 16)
						| ((unsigned long)csd[8] << 8)
						| csd[9]
						) + 1;
					return size << 10;
				}
				unsigned long size = (
					((unsigned long)(csd[6] & 0x03) << 10)
					| ((unsigned long)csd[7] << 2)
					| ((csd[8] & 0xC0) >> 6)
					) + 1;
				size <<= ((
					((csd[9] & 0x03) << 1)
					| ((csd[10] & 0x80) >> 7)
					) + 2);
				size <<= (csd[5] & 0x0F);
				return size >> 9;
			}
		} else {
			break;
		}
	}

	sdDeselectCard(pdrv);
	return 0;
}

/*
 * FATFS API
 * */

static DSTATUS ff_sd_initialize(uint8_t pdrv) {
	char token;
	unsigned int resp;
	unsigned int start;
	ardu_sdcard_t* card = s_cards[pdrv];

	if (!(card->status & STA_NOINIT)) {
		return card->status;
	}

	AcquireSPI card_locked(card);

	digitalWrite(card->ssPin, HIGH);
	for (uint8_t i = 0; i < 20; i++) {
		card->spi->transfer(0XFF);
	}

	// Fix mount issue - sdWait fail ignored before command GO_IDLE_STATE
	digitalWrite(card->ssPin, LOW);
	if (!sdWait(pdrv, 500)) {
		log_w("sdWait fail ignored, card initialize continues");
	}
	if (sdCommand(pdrv, GO_IDLE_STATE, 0, NULL) != 1) {
		sdDeselectCard(pdrv);
		log_w("GO_IDLE_STATE failed");
		goto unknown_card;
	}
	sdDeselectCard(pdrv);

	token = sdTransaction(pdrv, CRC_ON_OFF, 1, NULL);
	if (token == 0x5) {
		//old card maybe
		card->supports_crc = false;
	} else if (token != 1) {
		log_w("CRC_ON_OFF failed: %u", token);
		goto unknown_card;
	}

	if (sdTransaction(pdrv, SEND_IF_COND, 0x1AA, &resp) == 1) {
		if ((resp & 0xFFF) != 0x1AA) {
			log_w("SEND_IF_COND failed: %03X", resp & 0xFFF);
			goto unknown_card;
		}

		if (sdTransaction(pdrv, READ_OCR, 0, &resp) != 1 || !(resp & (1 << 20))) {
			log_w("READ_OCR failed: %X", resp);
			goto unknown_card;
		}

		start = millis();
		do {
			token = sdTransaction(pdrv, APP_OP_COND, 0x40100000, NULL);
		} while (token == 1 && (millis() - start) < 1000);

		if (token) {
			log_w("APP_OP_COND failed: %u", token);
			goto unknown_card;
		}

		if (!sdTransaction(pdrv, READ_OCR, 0, &resp)) {
			if (resp & (1 << 30)) {
				card->type = CARD_SDHC;
			} else {
				card->type = CARD_SD;
			}
		} else {
			log_w("READ_OCR failed: %X", resp);
			goto unknown_card;
		}
	} else {
		if (sdTransaction(pdrv, READ_OCR, 0, &resp) != 1 || !(resp & (1 << 20))) {
			log_w("READ_OCR failed: %X", resp);
			goto unknown_card;
		}

		start = millis();
		do {
			token = sdTransaction(pdrv, APP_OP_COND, 0x100000, NULL);
		} while (token == 0x01 && (millis() - start) < 1000);

		if (!token) {
			card->type = CARD_SD;
		} else {
			start = millis();
			do {
				token = sdTransaction(pdrv, SEND_OP_COND, 0x100000, NULL);
			} while (token != 0x00 && (millis() - start) < 1000);

			if (token == 0x00) {
				card->type = CARD_MMC;
			} else {
				log_w("SEND_OP_COND failed: %u", token);
				goto unknown_card;
			}
		}
	}

	if (card->type != CARD_MMC) {
		if (sdTransaction(pdrv, APP_CLR_CARD_DETECT, 0, NULL)) {
			log_w("APP_CLR_CARD_DETECT failed");
			goto unknown_card;
		}
	}

	if (card->type != CARD_SDHC) {
		if (sdTransaction(pdrv, SET_BLOCKLEN, 512, NULL) != 0x00) {
			log_w("SET_BLOCKLEN failed");
			goto unknown_card;
		}
	}

	card->sectors = sdGetSectorsCount(pdrv);

	/* 最大周波数を制限しない
	if (card->frequency > 25000000) {
		card->frequency = 25000000;
	}
	*/

	card->status &= ~STA_NOINIT;
	return card->status;

unknown_card:
	card->type = CARD_UNKNOWN;
	return card->status;
}

static DSTATUS ff_sd_status(uint8_t pdrv) {
	if (sdTransaction(pdrv, SEND_STATUS, 0, NULL)) {
		log_e("Check status failed");
		return STA_NOINIT;
	}
	return s_cards[pdrv]->status;
}

/*
 * Public methods
 * */

static uint8_t sdcard_init(uint8_t cs, SPIClass* spi, int hz) {

	uint8_t pdrv = 0xFF;
	if (ff_diskio_get_drive(&pdrv) != ESP_OK || pdrv == 0xFF) {
		return pdrv;
	}

	ardu_sdcard_t* card = (ardu_sdcard_t*)malloc(sizeof(ardu_sdcard_t));
	if (!card) {
		return 0xFF;
	}

	card->base_path = NULL;
	card->frequency = hz;
	card->spi = spi;
	card->ssPin = cs;

	card->supports_crc = true;
	card->type = CARD_NONE;
	card->status = STA_NOINIT;

	pinMode(card->ssPin, OUTPUT);
	digitalWrite(card->ssPin, HIGH);

	s_cards[pdrv] = card;

	return pdrv;
}

static uint8_t sdcard_unmount(uint8_t pdrv) {
	ardu_sdcard_t* card = s_cards[pdrv];
	if (pdrv >= FF_VOLUMES || card == NULL) {
		return 1;
	}
	card->status |= STA_NOINIT;
	card->type = CARD_NONE;

	return 0;
}

