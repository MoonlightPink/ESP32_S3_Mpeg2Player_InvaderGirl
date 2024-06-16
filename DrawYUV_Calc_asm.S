
#define VRAMWidth 320
#define VRAMHeight 240

// void DrawYUV_Calc_asm(u16* pVRAM0, const u8* pY0, const u8* pCr0, const u8* pCb0);

#define pVRAM0 a2 // u16*
#define pY0 a3 // const u8*
#define pCr0 a4 // const u8*
#define pCb0 a5 // const u8*

#define Imm0 a6 // const 0
#define Imm1 a7 // const 1

#define LoopCounterY a8 // int

#define CrR a9 // s16
#define CbrG a10 // s16
#define CbB a11 // s16
#define SrcYC a12 // u8 / s16
#define Level a13 // s32
#define RGB a14 // u16
#define Temp a15 // 32bits

.global DrawYUV_Calc_asm
    .section .text
    .align 4
DrawYUV_Calc_asm:
    entry sp, 32

	movi Imm0, 0
	movi Imm1, 1

	movi LoopCounterY, VRAMHeight / 2
DrawYUV_Calc_asm_LoopStartY:

    movi Temp, VRAMWidth / 2
    loop Temp, DrawYUV_Calc_asm_LoopEndX

	// s16 SrcCr = (*pCr0++)-0x80;
	l8ui SrcYC, pCr0, 0
	add pCr0, pCr0, Imm1
    movi Temp, 0x80
	sub SrcYC, SrcYC, Temp

	// int CrR = 1.402 * SrcCr;
	movi Temp, 359 // 1.402 * 256 = 358.912
	mul16s CrR, SrcYC, Temp
	srai CrR, CrR, 8

	// int CbrG = -0.71414 * SrcCr;
	movi Temp, -183 // -0.71414 * 256 = -182.81984
	mul16s CbrG, SrcYC, Temp

	// s16 SrcCb = (*pCb0++)-0x80;
	l8ui SrcYC, pCb0, 0
	add pCb0, pCb0, Imm1
    movi Temp, 0x80
	sub SrcYC, SrcYC, Temp

	// int CbrG += -0.34414 * SrcCb;
	movi Temp, -88 // -0.34414 * 256 = -88.09984
	mul16s Temp, SrcYC,Temp
	add CbrG, CbrG, Temp

	srai CbrG, CbrG, 8

	// int CbB = 1.772 * SrcCb;
	movi Temp, 454 // 1.772 * 256 = 453.632
	mul16s CbB, SrcYC,Temp
	srai CbB, CbB, 8

	// int Y = pY0[0];
	// DrawPixel(pVRAM0, 0, Y, CrR, CbrG, CbB);

	movi Temp, 0xff // for min check

	// Pixel X=0, Y=0.
	l8ui SrcYC, pY0, (VRAMWidth*0)+0

	add Level, SrcYC, CrR
	max Level, Level, Imm0
	min Level, Level, Temp
	srli RGB, Level, 3

	add Level, SrcYC, CbrG
	max Level, Level, Imm0
	min Level, Level, Temp
	srli Level, Level, 2
	slli Level, Level, 5
	or RGB, RGB, Level

	add Level, SrcYC, CbB
	max Level, Level, Imm0
	min Level, Level, Temp
	srli Level, Level, 3
	slli Level, Level, 11
	or RGB, RGB, Level

	s16i RGB, pVRAM0, 0*2

	// Pixel X=1, Y=0.
	l8ui SrcYC, pY0, (VRAMWidth*0)+1

	add Level, SrcYC, CrR
	max Level, Level, Imm0
	min Level, Level, Temp
	srli RGB, Level, 3

	add Level, SrcYC, CbrG
	max Level, Level, Imm0
	min Level, Level, Temp
	srli Level, Level, 2
	slli Level, Level, 5
	or RGB, RGB, Level

	add Level, SrcYC, CbB
	max Level, Level, Imm0
	min Level, Level, Temp
	srli Level, Level, 3
	slli Level, Level, 11
	or RGB, RGB, Level

	s16i RGB, pVRAM0, 1*2

	// pVRAM0+=VRAMWidth;
	movi Temp, VRAMWidth*2
	add pVRAM0, pVRAM0, Temp

	movi Temp, 0xff // for min check

	// Pixel X=0, Y=1.
	l8ui SrcYC, pY0, (VRAMWidth*1)+0

	add Level, SrcYC, CrR
	max Level, Level, Imm0
	min Level, Level, Temp
	srli RGB, Level, 3

	add Level, SrcYC, CbrG
	max Level, Level, Imm0
	min Level, Level, Temp
	srli Level, Level, 2
	slli Level, Level, 5
	or RGB, RGB, Level

	add Level, SrcYC, CbB
	max Level, Level, Imm0
	min Level, Level, Temp
	srli Level, Level, 3
	slli Level, Level, 11
	or RGB, RGB, Level

	s16i RGB, pVRAM0, 0*2

	// Pixel X=1, Y=1.
	l8ui SrcYC, pY0, (VRAMWidth*1)+1

	add Level, SrcYC, CrR
	max Level, Level, Imm0
	min Level, Level, Temp
	srli RGB, Level, 3

	add Level, SrcYC, CbrG
	max Level, Level, Imm0
	min Level, Level, Temp
	srli Level, Level, 2
	slli Level, Level, 5
	or RGB, RGB, Level

	add Level, SrcYC, CbB
	max Level, Level, Imm0
	min Level, Level, Temp
	srli Level, Level, 3
	slli Level, Level, 11
	or RGB, RGB, Level

	s16i RGB, pVRAM0, 1*2

	// pY0+=2;
	movi Temp, 2
	add pY0, pY0, Temp

	// pVRAM0-=VRAMWidth-2;
	movi Temp, (VRAMWidth-2)*2
	sub pVRAM0, pVRAM0, Temp


DrawYUV_Calc_asm_LoopEndX:

	// pY0+=VRAMWidth;
	movi Temp, VRAMWidth
	add pY0, pY0, Temp

	// pVRAM0+=VRAMWidth;
	movi Temp, VRAMWidth*2
	add pVRAM0, pVRAM0, Temp

	sub LoopCounterY, LoopCounterY, Imm1
	bnez LoopCounterY, DrawYUV_Calc_asm_LoopStartY

    retw
