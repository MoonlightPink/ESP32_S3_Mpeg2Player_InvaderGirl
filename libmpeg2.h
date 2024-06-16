#pragma once

extern "C" {
#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"
}

static const int buffer_size = 1 * 1024 * 1024;
static uint8_t* buffer;
static mpeg2dec_t* mpeg2dec;

extern "C" {
	static void* malloc_hook(unsigned size, mpeg2_alloc_t reason);
}

static void* malloc_hook(unsigned size, mpeg2_alloc_t reason) {
	Serial.println("ffmpeg: malloc(" + String(size) + "," + String(reason) + ")");

	void* buf;

	/*
	 * Invalid streams can refer to fbufs that have not been
	 * initialized yet. For example the stream could start with a
	 * picture type onther than I. Or it could have a B picture before
	 * it gets two reference frames. Or, some slices could be missing.
	 *
	 * Consequently, the output depends on the content 2 output
	 * buffers have when the sequence begins. In release builds, this
	 * does not matter (garbage in, garbage out), but in test code, we
	 * always zero all our output buffers to:
	 * - make our test produce deterministic outputs
	 * - hint checkergcc that it is fine to read from all our output
	 *   buffers at any time
	 */
	if ((int)reason < 0) {
		return NULL;
	}
	buf = malloc(size);// mpeg2_malloc(size, (mpeg2_alloc_t)-1);

	if (buf == NULL) {
		Serial.println("malloc failed.");
		while (1) { delay(1); }
	}

	if (reason == MPEG2_ALLOC_YUV || reason == MPEG2_ALLOC_CONVERTED) {
		memset(buf, 0, size);
	}

	return buf;
}

#define Limit8bits(v) if (v < 0x00) { v = 0x00; } else if (0xff < v) { v = 0xff; }

#define DrawPixel(p,x,y,_r,_g,_b) { \
int r=y+_r,g=y+_g,b=y+_b; \
if (r < 0x00) { r = 0x00; } \
if (0xff < r) { r = 0xff; } \
if (g < 0x00) { g = 0x00; } \
if (0xff < g) { g = 0xff; } \
if (b < 0x00) { b = 0x00; } \
if (0xff < b) { b = 0xff; } \
p[x] = (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11); \
}

#define DrawPixel16(p,x,rgb) { p[x] = rgb; }

extern "C" {
	void DrawYUV_Calc_asm(u16* pVRAM0, const u8* pY0, const u8* pCr0, const u8* pCb0);
	void DrawYUV_Table_asm(void* ptrs[12]);
};

static void DrawYUV(const int width, const int height, const int chroma_width, const int chroma_height, const u8* pY, const u8* pCb, const u8* pCr) {
	const u32 ms = millis();

	if ((width != (chroma_width * 2)) || (height != (chroma_height * 2))) {
		Serial.println("Invalid chroma size.");
		while (1) { delay(1); }
	}

	const int VRAMWidth = VGAMode.hRes;
	const int VRAMHeight = VGAMode.vRes;

	static bool Table = false;
	static s16 Table_CrR[0x100], Table_CrG[0x100], Table_CbG[0x100], Table_CbB[0x100];
	static u16* Table_pVRAM[MpegInfoHeight];
	static u16 Table_ClipR[0x200], Table_ClipG[0x200], Table_ClipB[0x200];
	static void* ptrs[12];
	if (!Table) {
		Table = true;
		Serial.println("DrawYUV: Create tables.");
		for (int idx = 0; idx < 0x100; idx++) {
			int v = idx - 0x80;
			Table_CrR[idx] = 1.402 * v;
			Table_CrG[idx] = -0.71414 * v;
			Table_CbG[idx] = -0.34414 * v;
			Table_CbB[idx] = 1.772 * v;
		}
		for (int y = 0; y < MpegInfoHeight; y++) {
			Table_pVRAM[y] = vga.dmaBuffer->getLineAddr16(y, vga.backBuffer);
		}
		for (int idx = -0x80; idx < 0x100 + 0x80; idx++) {
			int v = idx;
			Limit8bits(v);
			Table_ClipR[idx + 0x80] = v >> 3;
			Table_ClipG[idx + 0x80] = (v >> 2) << 5;
			Table_ClipB[idx + 0x80] = (v >> 3) << 11;
		}
		ptrs[0] = Table_pVRAM[0];
		ptrs[1] = NULL;
		ptrs[2] = NULL;
		ptrs[3] = NULL;
		ptrs[4] = Table_CrR;
		ptrs[5] = Table_CrG;
		ptrs[6] = Table_CbG;
		ptrs[7] = Table_CbB;
		ptrs[8] = &Table_ClipR[0x80];
		ptrs[9] = &Table_ClipG[0x80];
		ptrs[10] = &Table_ClipB[0x80];
		ptrs[11] = 0;
	}

	if (true) {
		if (false) {
			DrawYUV_Calc_asm(Table_pVRAM[0], pY, pCr, pCb); // 15ms
		}
		else {
			ptrs[1] = (u8*)pY;
			ptrs[2] = (u8*)pCr;
			ptrs[3] = (u8*)pCb;
			ptrs[11] = (void*)(VRAMHeight / 2); // •Ï”
			DrawYUV_Table_asm(ptrs); // 13ms
		}
		vga.show();
	}
	else {
		const u16* pClipR = &Table_ClipR[0x80];
		const u16* pClipG = &Table_ClipG[0x80];
		const u16* pClipB = &Table_ClipB[0x80];

		for (int y = 0; y < VRAMHeight / 2; y++) {
			const u8* pCr0 = &pCr[chroma_width * (y + 0)];
			const u8* pCb0 = &pCb[chroma_width * (y + 0)];
			const u8* pY0 = &pY[width * (y * 2 + 0)];
			const u8* pY1 = &pY[width * (y * 2 + 1)];
			u16* pVRAM0 = Table_pVRAM[y * 2 + 0];
			u16* pVRAM1 = Table_pVRAM[y * 2 + 1];
			if (false) { // C ŒvŽZ 50ms
				for (int x = 0; x < VRAMWidth / 2; x++) {
					const s16 SrcCr = (*pCr0++) - 0x80;
					const s16 SrcCb = (*pCb0++) - 0x80;

					const int CrR = 1.402 * SrcCr;
					const int CbrG = (-0.71414 * SrcCr) + (-0.34414 * SrcCb);
					const int CbB = 1.772 * SrcCb;

					{
						const int Y = pY0[0];
						DrawPixel(pVRAM0, 0, Y, CrR, CbrG, CbB);
					}
					{
						const int Y = pY0[1];
						DrawPixel(pVRAM0, 1, Y, CrR, CbrG, CbB);
					}
					pY0 += 2;
					pVRAM0 += 2;

					{
						const int Y = pY1[0];
						DrawPixel(pVRAM1, 0, Y, CrR, CbrG, CbB);
					}
					{
						const int Y = pY1[1];
						DrawPixel(pVRAM1, 1, Y, CrR, CbrG, CbB);
					}
					pY1 += 2;
					pVRAM1 += 2;
				}
			}
			else { // C ƒe[ƒuƒ‹ŽQÆ 14ms
				for (int x = 0; x < VRAMWidth / 2; x++) {
					const u8 SrcCr = *pCr0++;
					const u8 SrcCb = *pCb0++;

					const int CrR = Table_CrR[SrcCr];
					const u16* pR = &pClipR[CrR];
					const int CbrG = Table_CrG[SrcCr] + Table_CbG[SrcCb];
					const u16* pG = &pClipG[CbrG];
					const int CbB = Table_CbB[SrcCb];
					const u16* pB = &pClipB[CbB];

					{
						const int Y = pY0[0];
						DrawPixel16(pVRAM0, 0, pR[Y] | pG[Y] | pB[Y]);
					}
					{
						const int Y = pY0[1];
						DrawPixel16(pVRAM0, 1, pR[Y] | pG[Y] | pB[Y]);
					}
					pY0 += 2;
					pVRAM0 += 2;

					{
						const int Y = pY1[0];
						DrawPixel16(pVRAM1, 0, pR[Y] | pG[Y] | pB[Y]);
					}
					{
						const int Y = pY1[1];
						DrawPixel16(pVRAM1, 1, pR[Y] | pG[Y] | pB[Y]);
					}
					pY1 += 2;
					pVRAM1 += 2;
				}
			}
			VRAMFlush(y * 2 + 0);
			VRAMFlush(y * 2 + 1);
		}
	}

	//Serial.println("DrawYUV time: " + String(millis() - ms) + "ms.");
}

static u32 RawSectorIndex = 0;
static u32 PreloadedSectors = 0;

static bool libmpeg2_Init() {
	mpeg2dec = mpeg2_init();
	if (mpeg2dec == NULL) {
		Serial.println("mpeg2_init==NULL");
		return false;
	}
	mpeg2_accel(0);
	mpeg2_malloc_hooks(malloc_hook, NULL);

	buffer = (u8*)malloc(buffer_size);
	if (buffer == NULL) {
		Serial.println("malloc error.");
		return false;
	}

	{
		Serial.println("libmpeg2_Init: Read file.");
		const int ReqSectors = 10;// buffer_size / 512;
		Raw_ReadSectors(buffer, RawSectorIndex, ReqSectors);
		RawSectorIndex += ReqSectors;
		mpeg2_buffer(mpeg2dec, buffer, &buffer[ReqSectors * 512]);
	}

	return true;
}

static void libmpeg2_Close() {
	//mpeg2_close(mpeg2dec);

	free(buffer);
}

static bool libmpeg2_PreloadFile(bool ReplaceBuffer) {
	const int TotalSectorsCount = (MpegFileSize + 511) / 512;
	const int CanReadSectorsCount = TotalSectorsCount - RawSectorIndex;
	
	//Serial.println("CanReadSectorsCount: " + String(CanReadSectorsCount));

	if (CanReadSectorsCount == 0) { return false; }

	if (mpeg2dec->buf_end < &buffer[buffer_size]) {
		int ReqSectors = (&buffer[buffer_size] - mpeg2dec->buf_end) / 512;
		if (ReqSectors == 0) { Serial.println(String(__LINE__) + ": Internal error."); while (1) { delay(1); } }
		
		//Serial.println("Append to end: ReqSectors: " + String(ReqSectors));

		if (CanReadSectorsCount < ReqSectors) { ReqSectors = CanReadSectorsCount; }
		if (16 < ReqSectors) { ReqSectors = 16; } // ŒÅ‚Ü‚è–hŽ~

		Raw_ReadSectors(mpeg2dec->buf_end, RawSectorIndex, ReqSectors);
		RawSectorIndex += ReqSectors;
		mpeg2dec->buf_end += ReqSectors * 512;
		
		return true;
	}
	else {
		const int PreSectorsCount = (mpeg2dec->buf_start - buffer) / 512;
		int ReqSectors = PreSectorsCount - PreloadedSectors;
		if (ReqSectors == 0) { return false; }

		if (CanReadSectorsCount < ReqSectors) { ReqSectors = CanReadSectorsCount; }
		//Serial.println("Append to start: bufsize: " + String(mpeg2dec->buf_start - buffer) + ",\tReqSectors: " + String(ReqSectors));
		if (16 < ReqSectors) { ReqSectors = 16; } // ŒÅ‚Ü‚è–hŽ~

		Raw_ReadSectors(&buffer[PreloadedSectors * 512], RawSectorIndex, ReqSectors);
		RawSectorIndex += ReqSectors;
		PreloadedSectors += ReqSectors;

		if (ReplaceBuffer) {
			mpeg2_buffer(mpeg2dec, buffer, &buffer[PreloadedSectors * 512]);
			PreloadedSectors = 0;
		}

		return true;
	}
}

static bool libmpeg2_VideoEnabled = true;

static String PtrToStr(const void* p) { return "$" + String((u32)p, HEX); }

static bool libmpeg2_DrawFrame() {
	const u32 ms = millis();

	while (1) {
		mpeg2_state_t state = mpeg2_parse(mpeg2dec);
		switch (state) {
		case STATE_BUFFER:
		{
			//Serial.println("STATE_BUFFER");
			if (mpeg2dec->buf_end != mpeg2dec->buf_start) {
				Serial.println("Buffer not empty.");
				while (1) { delay(1); }
			}
			if (!libmpeg2_PreloadFile(true)) { return false; }
		}
		break;
		case STATE_SEQUENCE:
			/* might set nb fbuf, convert format, stride */
			/* might set fbufs */
			//Serial.println("STATE_SEQUENCE: width=" + String(info->sequence->width) + ", height=" + String(info->sequence->height) + ", chroma_width=" + String(info->sequence->chroma_width) + ", info->sequence->chroma_heihgt=" + String(info->sequence->chroma_height));
			//if (setup_result.convert && mpeg2_convert(mpeg2dec, setup_result.convert, NULL)) {}
			// require setup frame buffer.
			Serial.println("STATE_SEQUENCE");
			mpeg2_skip(mpeg2dec, false);
			break;
		case STATE_PICTURE:
			/* might skip */
			/* might set fbuf */
			// require setup frame buffer.
			//Serial.println("STATE_PICTURE");
			break;
		case STATE_SLICE:
		{
			//Serial.println("STATE_SLICE");
			/* draw current picture */
			/* might free frame buffer */
			//Serial.println("STATE_SLICE");
			//Serial.println("Decode time: " + String(millis() - ms) + "ms.");
			const mpeg2_info_t* info = mpeg2_info(mpeg2dec);
			if (info->display_fbuf) {
				//Serial.println("Draw: info->display_fbuf->buf=" + PtrToStr(info->display_fbuf->buf) + ", info->display_fbuf->id=" + PtrToStr(info->display_fbuf->id));
				if (libmpeg2_VideoEnabled) {
					DrawYUV(info->sequence->width, info->sequence->height, info->sequence->chroma_width, info->sequence->chroma_height, info->display_fbuf->buf[0], info->display_fbuf->buf[1], info->display_fbuf->buf[2]);
				}
			}
			if (info->discard_fbuf) {
				//Serial.println("Discard: info->display_fbuf->buf=" + PtrToStr(info->discard_fbuf->buf) + ", info->display_fbuf->id=" + PtrToStr(info->discard_fbuf->id));
			}
		}
		return true;
		case STATE_END:
		case STATE_INVALID_END:
			Serial.println("STATE_END");
			return false;
		default:
			//Serial.println("Unknown state.");
			break;
		}
	}
}

static void libmpeg2_SetVideoEnabled(bool f) {
	libmpeg2_VideoEnabled = f;
}