
#define VRAMWidth 320
#define VRAMHeight 240

// void DrawYUV_Table_asm(void* ptrs[12]);

#define ptrs a2
#define GetPtr(r, t) l32i r, ptrs, 4*t
#define SetPtr(r, t) s32i r, ptrs, 4*t

#define EPtr_pVRAM0 0 // u16*
#define EPtr_pY 1 // const u8*
#define EPtr_pCr 2 // const u8*
#define EPtr_pCb 3 // const u8*
#define EPtr_Table_CrR 4 // const s16*
#define EPtr_Table_CrG 5 // const s16*
#define EPtr_Table_CbG 6 // const s16*
#define EPtr_Table_CbB 7 // const s16*
#define EPtr_Table_ClipR 8 // const u8*
#define EPtr_Table_ClipG 9 // const u8*
#define EPtr_Table_ClipB 10 // const u8*
#define EPtr_LoopCounterY 11 // ïœêî

#define pTable_C a3 // const s16*
#define pTable_Clip a3 // const u8*
#define LoopCounterY a3 // int

#define pVRAM0 a4 // u16*
#define pY0 a5 // const u8*
#define pCr0 a6 // const u8*
#define pCb0 a7 // const u8*

#define Imm1 a8 // const 1

#define CrR a9 // s16
#define CbrG a10 // s16
#define CbB a11 // s16
#define pClipR a9 // u16*
#define pClipG a10 // u16*
#define pClipB a11 // u16*

#define SrcYC a12 // u8 / s16
#define Level a13 // s32
#define RGB a14 // u16

#define Temp a15 // 32bits

.global DrawYUV_Table_asm
    .section .text
    .align 4
DrawYUV_Table_asm:
    entry sp, 32

	GetPtr(pVRAM0, EPtr_pVRAM0)
	GetPtr(pY0, EPtr_pY)
	GetPtr(pCr0, EPtr_pCr)
	GetPtr(pCb0, EPtr_pCb)

	movi Imm1, 1

DrawYUV_Table_asm_LoopStartY:

    movi Temp, VRAMWidth / 2
    loop Temp, DrawYUV_Table_asm_LoopEndX

	// s16 SrcCr = (*pCr0++)-0x80;
	l8ui SrcYC, pCr0, 0
	add pCr0, pCr0, Imm1

	// int CrR = 1.402 * SrcCr;
	GetPtr(pTable_C, EPtr_Table_CrR)
	addx2 pTable_C, SrcYC, pTable_C
	l16si CrR, pTable_C, 0

	// int CbrG = -0.71414 * SrcCr;
	GetPtr(pTable_C, EPtr_Table_CrG)
	addx2 pTable_C, SrcYC, pTable_C
	l16si CbrG, pTable_C, 0

	// s16 SrcCb = (*pCb0++)-0x80;
	l8ui SrcYC, pCb0, 0
	add pCb0, pCb0, Imm1

	// int CbrG += -0.34414 * SrcCb;
	GetPtr(pTable_C, EPtr_Table_CbG)
	addx2 pTable_C, SrcYC, pTable_C
	l16si Temp, pTable_C, 0
	add CbrG, CbrG, Temp

	// int CbB = 1.772 * SrcCb;
	GetPtr(pTable_C, EPtr_Table_CbB)
	addx2 pTable_C, SrcYC, pTable_C
	l16si CbB, pTable_C, 0

	GetPtr(pTable_Clip, EPtr_Table_ClipR)
	addx2 pClipR, CrR, pTable_Clip
	GetPtr(pTable_Clip, EPtr_Table_ClipG)
	addx2 pClipG, CbrG, pTable_Clip
	GetPtr(pTable_Clip, EPtr_Table_ClipB)
	addx2 pClipB, CbB, pTable_Clip

	// int Y = pY0[0];
	// DrawPixel(pVRAM0, 0, Y, CrR, CbrG, CbB);

	// Pixel X=0, Y=0.
	l8ui SrcYC, pY0, (VRAMWidth*0)+0

	addx2 Temp, SrcYC, pClipR
	l16ui RGB, Temp, 0

	addx2 Temp, SrcYC, pClipG
	l16ui Level, Temp, 0
	or RGB, RGB, Level

	addx2 Temp, SrcYC, pClipB
	l16ui Level, Temp, 0
	or RGB, RGB, Level

	s16i RGB, pVRAM0, 0*2

	// Pixel X=1, Y=0.
	l8ui SrcYC, pY0, (VRAMWidth*0)+1

	addx2 Temp, SrcYC, pClipR
	l16ui RGB, Temp, 0

	addx2 Temp, SrcYC, pClipG
	l16ui Level, Temp, 0
	or RGB, RGB, Level

	addx2 Temp, SrcYC, pClipB
	l16ui Level, Temp, 0
	or RGB, RGB, Level

	s16i RGB, pVRAM0, 1*2

	// pVRAM0+=VRAMWidth;
	movi Temp, VRAMWidth*2
	add pVRAM0, pVRAM0, Temp

	// Pixel X=0, Y=1.
	l8ui SrcYC, pY0, (VRAMWidth*1)+0

	addx2 Temp, SrcYC, pClipR
	l16ui RGB, Temp, 0

	addx2 Temp, SrcYC, pClipG
	l16ui Level, Temp, 0
	or RGB, RGB, Level

	addx2 Temp, SrcYC, pClipB
	l16ui Level, Temp, 0
	or RGB, RGB, Level

	s16i RGB, pVRAM0, 0*2

	// Pixel X=1, Y=1.
	l8ui SrcYC, pY0, (VRAMWidth*1)+1

	addx2 Temp, SrcYC, pClipR
	l16ui RGB, Temp, 0

	addx2 Temp, SrcYC, pClipG
	l16ui Level, Temp, 0
	or RGB, RGB, Level

	addx2 Temp, SrcYC, pClipB
	l16ui Level, Temp, 0
	or RGB, RGB, Level

	s16i RGB, pVRAM0, 1*2

	// pY0+=2;
	movi Temp, 2
	add pY0, pY0, Temp

	// pVRAM0-=VRAMWidth-2;
	movi Temp, (VRAMWidth-2)*2
	sub pVRAM0, pVRAM0, Temp

DrawYUV_Table_asm_LoopEndX:

	// pY0+=VRAMWidth;
	movi Temp, VRAMWidth
	add pY0, pY0, Temp

	// pVRAM0+=VRAMWidth;
	movi Temp, VRAMWidth*2
	add pVRAM0, pVRAM0, Temp

	GetPtr(LoopCounterY, EPtr_LoopCounterY)
	sub LoopCounterY, LoopCounterY, Imm1
	SetPtr(LoopCounterY, EPtr_LoopCounterY)
	bnez LoopCounterY, DrawYUV_Table_asm_LoopStartY

    retw
