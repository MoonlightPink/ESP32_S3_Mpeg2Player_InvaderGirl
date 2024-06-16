#pragma once

#include "font-5x8.h"
#include "font-6x8.h"
#include "font-8x8.h"
#include "font-9x8.h"
#include "font-9x16.h"
#include "font-12x24.h"
#include "font-16x32.h"

static int Console_Line = 0;
static int Console_FontHeight = 58;

static u16 Console_TextColor_Bright = vga.rgb(0xFF, 0xF0, 0xFF);
static u16 Console_TextColor_Shadow = vga.rgb(0x20, 0x10, 0x80);

static void ConsoleSetLine(int Line) { Console_Line = Line; }
static void ConsoleSetFontHeight(int h) { Console_FontHeight = h; }

static void ConsoleScroll(int Height) {
	Console_Line += Height;

	if (VGAMode.vRes < Console_Line) {
		const int v = Console_Line - VGAMode.vRes;
		for (int y = 0; y < VGAMode.vRes - v; y++) {
			switch (vga.bits) {
			case 8: {
				const u8* psrc = vga.dmaBuffer->getLineAddr8(y + v, vga.backBuffer);
				u8* pdst = vga.dmaBuffer->getLineAddr8(y, vga.backBuffer);
				for (int x = 0; x < VGAMode.hRes; x++) {
					*pdst++ = *psrc++;
				}
			}break;
			case 16: {
				const u16* psrc = vga.dmaBuffer->getLineAddr16(y + v, vga.backBuffer);
				u16* pdst = vga.dmaBuffer->getLineAddr16(y, vga.backBuffer);
				for (int x = 0; x < VGAMode.hRes; x++) {
					*pdst++ = *psrc++;
				}
			}break;
			default: Serial.println("Illigal VGA bits mode. vga.bits" + String(vga.bits)); while (true) { delay(1); }
			}
			VRAMFlush(y);
		}
		for (int y = VGAMode.vRes - v; y < VGAMode.vRes; y++) {
			switch (vga.bits) {
			case 8: {
				u8* pdst = vga.dmaBuffer->getLineAddr8(y, vga.backBuffer);
				for (int x = 0; x < VGAMode.hRes; x++) {
					*pdst++ = 0x00;
				}
			}break;
			case 16: {
				u16* pdst = vga.dmaBuffer->getLineAddr16(y, vga.backBuffer);
				for (int x = 0; x < VGAMode.hRes; x++) {
					*pdst++ = 0x00;
				}
			}break;
			default: Serial.println("Illigal VGA bits mode. vga.bits" + String(vga.bits)); while (true) { delay(1); }
			}
			VRAMFlush(y);
		}
		Console_Line -= v;
	}

	Console_Line -= Height;
}

static void ConsoleWriteLine(int Line, String Text) {
	int DataBits;
	const u16* pFont;
	int Width, Height, PaddingY;

	switch (Console_FontHeight) {
	case 58: DataBits = 8; pFont = console_font_5x8; Width = 5; Height = 8; PaddingY = 0; break;
	case 68: DataBits = 8; pFont = console_font_6x8; Width = 6; Height = 8; PaddingY = 0; break;
	case 88: DataBits = 8; pFont = console_font_8x8; Width = 8; Height = 8; PaddingY = 0; break;
	case 98: DataBits = 16; pFont = console_font_9x8; Width = 9; Height = 8; PaddingY = 1; break;
	case 16: DataBits = 16; pFont = console_font_9x16; Width = 9; Height = 16; PaddingY = 0; break;
	case 24: DataBits = 16; pFont = console_font_12x24; Width = 12; Height = 24; PaddingY = 0; break;
	case 32: DataBits = 16; pFont = console_font_16x32; Width = 16; Height = 32; PaddingY = 0; break;
	default: Serial.println("Illigal font height. Console_FontHeight:" + String(Console_FontHeight));
	}

	for (int y = 0; y < Height + 1; y++) {
		for (int x = 0; x < Width * Text.length() + 1; x++) {
			vga.dot(x, Line + y, vga.rgb(0x00, 0x00, 0x00));
		}
	}
	for (int chidx = 0; chidx < Text.length(); chidx++) {
		u8 ch = Text[chidx];
		const u16* pbm = &pFont[Height * ch];
		for (int y = 0; y < Height; y++) {
			u16 bm = *pbm++;
			if (DataBits == 8) { bm <<= 8; }
			for (int x = 0; x < Width; x++) {
				if ((bm & 0x8000) != 0) {
					vga.dot(chidx * Width + x + 0, Line + y + 0, Console_TextColor_Bright);
					vga.dot(chidx * Width + x + 1, Line + y + 1, Console_TextColor_Shadow);
				}
				bm <<= 1;
			}
		}
	}
	for (int y = 0; y < Height + 1; y++) {
		VRAMFlush(Line + y);
	}
}

static void ConsoleWriteLine(String Text) {
	int DataBits;
	const u16* pFont;
	int Width, Height, PaddingY;

	switch (Console_FontHeight) {
	case 58: DataBits = 8; pFont = console_font_5x8; Width = 5; Height = 8; PaddingY = 0; break;
	case 68: DataBits = 8; pFont = console_font_6x8; Width = 6; Height = 8; PaddingY = 0; break;
	case 88: DataBits = 8; pFont = console_font_8x8; Width = 8; Height = 8; PaddingY = 0; break;
	case 98: DataBits = 16; pFont = console_font_9x8; Width = 9; Height = 8; PaddingY = 1; break;
	case 16: DataBits = 16; pFont = console_font_9x16; Width = 9; Height = 16; PaddingY = 0; break;
	case 24: DataBits = 16; pFont = console_font_12x24; Width = 12; Height = 24; PaddingY = 0; break;
	case 32: DataBits = 16; pFont = console_font_16x32; Width = 16; Height = 32; PaddingY = 0; break;
	default: Serial.println("Illigal font height. Console_FontHeight:" + String(Console_FontHeight)); while (true) { delay(1); }
	}

	ConsoleScroll(Height + PaddingY + 1);
	ConsoleWriteLine(Console_Line, Text);
	Console_Line += Height + PaddingY + 1;
}