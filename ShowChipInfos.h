#pragma once

#include "esp_chip_info.h"
#include "esp_mac.h"

static void ShowChipInfos() {
	char buf[64];

	uint64_t chipid;
	chipid = ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
	snprintf(buf, 64, "ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32));//print High 2 bytes
	ConsoleWriteLine(buf);

	snprintf(buf, 64, "Chip Revision %d", ESP.getChipRevision());
	ConsoleWriteLine(buf);
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	snprintf(buf, 64, "Number of Core: %d", chip_info.cores);
	ConsoleWriteLine(buf);
	snprintf(buf, 64, "CPU Frequency: %d MHz", ESP.getCpuFreqMHz());
	ConsoleWriteLine(buf);
	snprintf(buf, 64, "Flash Chip Size = %d byte", ESP.getFlashChipSize());
	ConsoleWriteLine(buf);
	snprintf(buf, 64, "Flash Frequency = %d Hz", ESP.getFlashChipSpeed());
	ConsoleWriteLine(buf);
	snprintf(buf, 64, "ESP-IDF version = %s", esp_get_idf_version());
	ConsoleWriteLine(buf);
	//利用可能なヒープのサイズを取得
	snprintf(buf, 64, "Available Heap Size= %d", esp_get_free_heap_size());
	ConsoleWriteLine(buf);
	//利用可能な内部ヒープのサイズを取得
	snprintf(buf, 64, "Available Internal Heap Size = %d", esp_get_free_internal_heap_size());
	ConsoleWriteLine(buf);
	//これまでに利用可能だった最小ヒープを取得します
	snprintf(buf, 64, "Min Free Heap Ever Avg Size = %d", esp_get_minimum_free_heap_size());
	ConsoleWriteLine(buf);

	return;

	uint8_t mac0[6];
	esp_efuse_mac_get_default(mac0);
	snprintf(buf, 64, "Default Mac Address = %02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
	ConsoleWriteLine(buf);

	uint8_t mac3[6];
	esp_read_mac(mac3, ESP_MAC_WIFI_STA);
	snprintf(buf, 64, "[Wi-Fi Station] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X", mac3[0], mac3[1], mac3[2], mac3[3], mac3[4], mac3[5]);
	ConsoleWriteLine(buf);

	uint8_t mac4[7];
	esp_read_mac(mac4, ESP_MAC_WIFI_SOFTAP);
	snprintf(buf, 64, "[Wi-Fi SoftAP] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X", mac4[0], mac4[1], mac4[2], mac4[3], mac4[4], mac4[5]);
	ConsoleWriteLine(buf);

	uint8_t mac5[6];
	esp_read_mac(mac5, ESP_MAC_BT);
	snprintf(buf, 64, "[Bluetooth] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X", mac5[0], mac5[1], mac5[2], mac5[3], mac5[4], mac5[5]);
	ConsoleWriteLine(buf);

	uint8_t mac6[6];
	esp_read_mac(mac6, ESP_MAC_ETH);
	snprintf(buf, 64, "[Ethernet] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X", mac6[0], mac6[1], mac6[2], mac6[3], mac6[4], mac6[5]);
	ConsoleWriteLine(buf);
}
