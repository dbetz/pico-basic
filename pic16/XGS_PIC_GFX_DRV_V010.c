///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright Nurve Networks LLC 2008
//
// Filename: XGS_PIC_GFX_DRV_V010.H
//
// Original Author: Joshua Hintze
//
// Last Modified: 9.6.08
//
// Description: Implementation file for Graphics lib.
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// First and foremost, include the chip definition header
#include "p24HJ256GP206.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// include local XGS API header files
#include "XGS_PIC_SYSTEM_V010.h"
#include "XGS_PIC_GFX_DRV_V010.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Font base index definitions for getting at specific characters.
// This must mach what the font data set has setup
#define TMAP_BASE_BLANK 	0


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Bitmap engine needed variables
int g_ScreenWidth;					// Screen width of the loaded graphics driver
int g_ScreenHeight;					// Screen height of the loaded graphics driver
int g_ScreenBPP;					// Number of bits per pixel in the loaded graphics driver
unsigned char *g_CurrentVRAMBuff;   // 16-bit pointer to the vram that is meant to be drawn
unsigned short *g_AttribTable;   	// 16-bit pointer to a int array that contains the index of the 8x8
									// blocks that maps to the color tables

// Needed by both bitmap and tile engines
unsigned char *g_ColorTable;        // 16-bit pointer to the array of color tables.

// clipping regions
int g_ClipTop;
int g_ClipBottom;
int g_ClipLeft;
int g_ClipRight;

// Tile map engine variables needed
unsigned char *g_TileSet; 			// 16-bit pointer to a Character set as 8x8 with 1 byte per pixel
unsigned char *g_TileMapBasePtr;	// 16-bit pointer to the tile map
Sprite *g_SpriteList;				// 16-bit pointer to sprite table records

// Tile map text mode global values
int g_Tmap_Cursor_X = 0;			// Text mode cursor x position
int g_Tmap_Cursor_Y = 0;			// Text mode cursor y position
int g_TileMapWidth  = 10;			// Used to store the tile map width for text console modes
int g_TileMapHeight = 10;			// Used to store the tile map height for text console modes

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXTERNALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Used for clipping a line to the viewport
int Clip_Line(int *x1,int *y1,int *x2, int *y2);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Initializes the graphics engine
void GFX_InitBitmap(int ScreenWidth, int ScreenHeight, int ScreenBPP, unsigned char *VRAMPtr,
			  		unsigned char *Palettes, unsigned short *PaletteMap)
{
	// Assign all the pointers.
	g_ScreenWidth = ScreenWidth;
	g_ScreenHeight = ScreenHeight;
	g_ScreenBPP = ScreenBPP;
	g_CurrentVRAMBuff = VRAMPtr;
	g_ColorTable = Palettes;
	g_AttribTable = PaletteMap;

	// Setup default clipping regions
	g_ClipTop = 0;
	g_ClipBottom = ScreenHeight-1;
	g_ClipLeft = 0;
	g_ClipRight = ScreenWidth-1;
}

// Initializes the graphics engine
void GFX_InitTile(unsigned char *TileData, unsigned char *TileMap, unsigned char *Palettes, Sprite *SpriteTable)
{
	g_TileSet = TileData;
	g_TileMapBasePtr = TileMap;
	g_ColorTable = Palettes;
	g_SpriteList = SpriteTable;
}


// Starts the timer1 interrupt which calls our assembly video driver code
void GFX_StartDrawing(int ScreenType)
{
	// Configure PORTB graphics section as an output
	PORTB &= ~(0x00FF);				// Set bits to 0
	TRISB &= ~(0x00FF);				// Set pins to output

	// Configure PORTF saturation controls for NTSC
	PORTF &= ~(0x03);				// Turn lower two bits to off RF0, RF1
	TRISF &= ~(0x03);				// Turn lower two bits to output

	TMR1 = 0;						// Clear the current timer value
	if(ScreenType == SCREEN_TYPE_NTSC)
		PR1 = 2723;						// Period = (2723+1 counts = 227 pixel clocks * 12) in a 3.57*12 Clock for NTSC
	else
		PR1 = 1399;						// Period = 1399+1 counts = 31.77 us using 22.69 nS Clock for VGA

	T1CONbits.TCS = 0;				// Use internal clock source
	IPC0bits.T1IP = 7;				// Set interrupt priority to 7 (0->7)
	IFS0bits.T1IF = 0;				// Clear interrupt flag in status register
	IEC0bits.T1IE = 1;				// Enable Timer1 interrupt
	T1CONbits.TON = 1;				// Turn the timer on
}


// Sets the region all clipped functions will be clipped against
void GFX_SetClipRegion(int Top, int Bottom, int Left, int Right)
{
	g_ClipTop = Top;
	g_ClipBottom = Bottom;
	g_ClipLeft = Left;
	g_ClipRight = Right;
}


// Returns the color of a pixel. Useful for collision detections
unsigned char GFX_GetPixel_2BPP(int x, int y, unsigned char *VRAMPtr)
{
	// Calculate the shift amount to get to the individual pixel
	int ShiftAmount = 6 - (x & 0x03)*2;
	unsigned int Index = (g_ScreenWidth>>2)*y + (x>>2);

	// We need to mask out the previous pixel
	unsigned char CurValue = VRAMPtr[Index];

	return ((CurValue >> ShiftAmount) & 0x03);
}


// Returns the color of a pixel. Useful for collision detections
unsigned char GFX_GetPixel_4BPP(int x, int y, unsigned char *VRAMPtr)
{
	// Calculate the shift amount to get to the individual pixel
	int ShiftAmount = (x & 0x01) ? 0 : 4;
	unsigned int Index = (g_ScreenWidth>>1)*y + (x>>1);

	// We need to mask out the previous pixel
	unsigned char CurValue = VRAMPtr[Index];

	return ((CurValue >> ShiftAmount) & 0x0F);
}



// Plots a single pixel non-clipped to the viewport
void GFX_Plot_2BPP(int x, int y, unsigned char Color, unsigned char *VRAMPtr)
{
	// Calculate the shift amount to get to the individual pixel
	int ShiftAmount = 6 - (x & 0x03)*2;
	unsigned int Index = (g_ScreenWidth>>2)*y + (x>>2);

	// We need to mask out the previous pixel and load in the new one
	// Originally this was a signle line of code, however the later compilers had 
	// problems with it, although the NEWEST compiler's errata says it has been fixed?
	unsigned char CurValue = VRAMPtr[Index];
	CurValue &= ~(0x03 << ShiftAmount);
	CurValue |= (Color << ShiftAmount);
	VRAMPtr[Index] = CurValue;
}


// Plots a single pixel non-clipped to the viewport
void GFX_Plot_4BPP(int x, int y, unsigned char Color, unsigned char *VRAMPtr)
{
	// Calculate the shift amount to get to the individual pixel
	int ShiftAmount = (x & 0x01) ? 0 : 4;
	unsigned int Index = (g_ScreenWidth>>1)*y + (x>>1);

	// We need to mask out the previous pixel
	unsigned char CurValue = VRAMPtr[Index];
	CurValue &= ~(0x0F << ShiftAmount);
	CurValue |= (Color << ShiftAmount);
	VRAMPtr[Index] = CurValue;
}


// Plots a single pixel clipped to the viewport
void GFX_Plot_Clip_2BPP(int x, int y, unsigned char Color, unsigned char *VRAMPtr)
{
	// Check for clipping against the assigned regions
	if(x < g_ClipLeft) return;
	if(x > g_ClipRight) return;
	if(y < g_ClipTop) return;
	if(y > g_ClipBottom) return;


	// Calculate the shift amount to get to the individual pixel
	int ShiftAmount = 6 - (x & 0x03)*2;
	unsigned int Index = (g_ScreenWidth>>2)*y + (x>>2);

	// We need to mask out the previous pixel
	unsigned char CurValue = VRAMPtr[Index];
	CurValue &= ~(0x03 << ShiftAmount);
	CurValue |= (Color << ShiftAmount);
	VRAMPtr[Index] = CurValue;
}


// Plots a single pixel clipped to the viewport
void GFX_Plot_Clip_4BPP(int x, int y, unsigned char Color, unsigned char *VRAMPtr)
{
	// Check for clipping against the assigned regions
	if(x < g_ClipLeft) return;
	if(x > g_ClipRight) return;
	if(y < g_ClipTop) return;
	if(y > g_ClipBottom) return;

	// Calculate the shift amount to get to the individual pixel
	int ShiftAmount = (x & 0x01) ? 0 : 4;
	unsigned int Index = (g_ScreenWidth>>1)*y + (x>>1);

	// We need to mask out the previous pixel
	unsigned char CurValue = VRAMPtr[Index];
	CurValue &= ~(0x0F << ShiftAmount);
	CurValue |= (Color << ShiftAmount);
	VRAMPtr[Index] = CurValue;
}



// Draws a clipped vertical line from (y1) to (y2)
void GFX_VLine_2BPP(int x, int y1, int y2, unsigned char Color, unsigned char *VRAMPtr)
{
	// Clip the coordinates
	if(x < g_ClipLeft) return;
	if(x > g_ClipRight) return;

	// Check for swap since we always draw top to bottom
	if(y1 > y2)
	{
		int SwapTmp;
		SwapTmp = y1;
		y1 = y2;
		y2 = SwapTmp;
	}

	if(y1 > g_ClipBottom) return;
	if(y2 < g_ClipTop) return;

	if(y1 < g_ClipTop) y1 = g_ClipTop;
	if(y2 > g_ClipBottom) y2 = g_ClipBottom;

	// Calculate the shift amount to get to the individual pixel
	int ShiftAmount = 6 - (x & 0x03)*2;
	unsigned int Index = (g_ScreenWidth>>2)*y1 + (x>>2);

	unsigned char MaskValue = ~(0x03 << ShiftAmount);
	unsigned char ColorValue = (Color << ShiftAmount);
	unsigned int ScreenPitch = g_ScreenWidth>>2;
	unsigned char CurValue;

	// Now loop through and draw out all the pixels in the V-line
	for(;y1 <= y2; y1++)
	{
		CurValue = VRAMPtr[Index];
		CurValue &= MaskValue;
		CurValue |= ColorValue;
		VRAMPtr[Index] = CurValue;
		Index += ScreenPitch;
	}
}


// Draws a clipped vertical line from (y1) to (y2)
void GFX_VLine_4BPP(int x, int y1, int y2, unsigned char Color, unsigned char *VRAMPtr)
{

	// Clip the coordinates
	if(x < g_ClipLeft) return;
	if(x > g_ClipRight) return;

	// Check for swap since we always draw top to bottom
	if(y1 > y2)
	{
		int SwapTmp;
		SwapTmp = y1;
		y1 = y2;
		y2 = SwapTmp;
	}

	if(y1 > g_ClipBottom) return;
	if(y2 < g_ClipTop) return;

	if(y1 < g_ClipTop) y1 = g_ClipTop;
	if(y2 > g_ClipBottom) y2 = g_ClipBottom;


	// Calculate the shift amount to get to the individual pixel
	int ShiftAmount = (x & 0x01) ? 0 : 4;
	unsigned int Index = (g_ScreenWidth>>1)*y1 + (x>>1);

	unsigned char MaskValue = ~(0x0F << ShiftAmount);
	unsigned char ColorValue = (Color << ShiftAmount);
	unsigned int ScreenPitch = g_ScreenWidth>>1;
	unsigned char CurValue;

	// Now loop through and draw out all the pixels in the V-line
	for(;y1 <= y2; y1++)
	{
		CurValue = VRAMPtr[Index];
		CurValue &= MaskValue;
		CurValue |= ColorValue;
		VRAMPtr[Index] = CurValue;
		Index += ScreenPitch;
	}
}



// Draws a clipped horizontal line from (x1) to (x2)
void GFX_HLine_2BPP(int x1, int x2, int y, unsigned char Color, unsigned char *VRAMPtr)
{

	// Quick check bounds
	if(y < g_ClipTop) return;
	if(y > g_ClipBottom) return;

	// Always draw left to right
	if(x2 < x1)
	{
		int TmpSwp = x1;
		x1 = x2;
		x2 = TmpSwp;
	}

	// Is it outside?
	if(x1 > g_ClipRight) return;
	if(x2 < g_ClipLeft) return;

	if(x1 < g_ClipLeft) x1 = g_ClipLeft;
	if(x2 > g_ClipRight) x2 = g_ClipRight;

	// Set up some variables we'll use
	int Pixel_dx = x2 - x1;

	int Index_x1 = (g_ScreenWidth>>2)*y + (x1>>2);
	int Index_x2 = (g_ScreenWidth>>2)*y + (x2>>2);
	int Index_dx = Index_x2-Index_x1;

	static unsigned int ColorComposites[4] = { 0x0000, 0x5555, 0xAAAA, 0xFFFF };
	unsigned int ColorComp1 = ColorComposites[Color];
	unsigned int ColorComp2 = ColorComposites[Color];

	unsigned int Mask1 = 0xFFFF;
	unsigned int Mask2 = 0xFFFF;

	unsigned int ShiftRightAmount = (x1 & 0x03)<<1;

	unsigned char CurValue;

	if(Pixel_dx < 4)
	{
		// Line crosses 0,1 pixel boundries
		unsigned int ShiftLeftAmount = ((1-Index_dx)<<3) + ((3-(x2 & 0x03))<<1);

		// Set up our masking operation
		Mask1 = Mask1 >> ShiftRightAmount;
		ColorComp1 = ColorComp1 >> ShiftRightAmount;

		Mask2 = Mask2 << ShiftLeftAmount;
		ColorComp2 = ColorComp2 << ShiftLeftAmount;

		Mask1 &= Mask2;
		ColorComp1 &= ColorComp2;

		// Take old colors out and put new colors in
		CurValue = VRAMPtr[Index_x1];
		CurValue &= ~(Mask1 >> 8);
		CurValue |= (ColorComp1 >> 8);
		VRAMPtr[Index_x1] = CurValue;

		CurValue = VRAMPtr[Index_x2];
		CurValue &= ~(Mask1 & 0xFF);
		CurValue |= (ColorComp1 & 0xFF);
		VRAMPtr[Index_x2] = CurValue;
	}
	else
	{
		// Line crosses 1 or more pixel boundries
		unsigned int ColorInside = ColorComposites[Color];
		unsigned int ShiftLeftAmount =  (3-(x2 & 0x03))<<1;

		// Set up our masking operation
		Mask1 = Mask1 >> ShiftRightAmount;
		ColorComp1 = ColorComp1 >> ShiftRightAmount;

		Mask2 = Mask2 << ShiftLeftAmount;
		ColorComp2 = ColorComp2 << ShiftLeftAmount;

		Mask1 &= Mask2;
		ColorComp1 &= ColorComp2;

		// Fill up the outside colors first
		// Take old colors out and put new colors in
		CurValue = VRAMPtr[Index_x1];
		CurValue &= ~(Mask1 >> 8);
		CurValue |= (ColorComp1 >> 8);
		VRAMPtr[Index_x1] = CurValue;

		CurValue = VRAMPtr[Index_x2];
		CurValue &= ~(Mask1 & 0xFF);
		CurValue |= (ColorComp1 & 0xFF);
		VRAMPtr[Index_x2] = CurValue;

		// Now loop through and fill up the Inside ones
		int i;
		for(i = 1; i < Index_dx; i++)
			VRAMPtr[Index_x1+i] = (ColorInside & 0xFF);
	}
}


// Draws a clipped horizontal line from (x1) to (x2)
void GFX_HLine_4BPP(int x1, int x2, int y, unsigned char Color, unsigned char *VRAMPtr)
{
	// Quick check bounds
	if(y < g_ClipTop) return;
	if(y > g_ClipBottom) return;

	// Always draw left to right
	if(x2 < x1)
	{
		int TmpSwp = x1;
		x1 = x2;
		x2 = TmpSwp;
	}

	// Is it outside?
	if(x1 > g_ClipRight) return;
	if(x2 < g_ClipLeft) return;

	if(x1 < g_ClipLeft) x1 = g_ClipLeft;
	if(x2 > g_ClipRight) x2 = g_ClipRight;


	// Set up some variables we'll use
	int Pixel_dx = x2 - x1;

	int Index_x1 = (g_ScreenWidth>>1)*y + (x1>>1);
	int Index_x2 = (g_ScreenWidth>>1)*y + (x2>>1);
	int Index_dx = Index_x2-Index_x1;

	static unsigned int ColorComposites[16] = {
		0x0000, 0x1111, 0x2222, 0x3333,
		0x4444, 0x5555, 0x6666, 0x7777,
		0x8888, 0x9999, 0xAAAA, 0xBBBB,
		0xCCCC, 0xDDDD, 0xEEEE, 0xFFFF,
	};
	unsigned int ColorComp1 = ColorComposites[Color];
	unsigned int ColorComp2 = ColorComposites[Color];

	unsigned int Mask1 = 0xFFFF;
	unsigned int Mask2 = 0xFFFF;

	unsigned int ShiftRightAmount = (x1 & 0x01)<<2;

	unsigned char CurValue;

	if(Pixel_dx < 2)
	{
		// Line crosses 0,1 pixel boundries
		unsigned int ShiftLeftAmount = ((1-Index_dx)<<3) + ((1-(x2 & 0x01))<<2);

		// Set up our masking operation
		Mask1 = Mask1 >> ShiftRightAmount;
		ColorComp1 = ColorComp1 >> ShiftRightAmount;

		Mask2 = Mask2 << ShiftLeftAmount;
		ColorComp2 = ColorComp2 << ShiftLeftAmount;

		Mask1 &= Mask2;
		ColorComp1 &= ColorComp2;

		// Take old colors out and put new colors in
		CurValue = VRAMPtr[Index_x1];
		CurValue &= ~(Mask1 >> 8);
		CurValue |= (ColorComp1 >> 8);
		VRAMPtr[Index_x1] = CurValue;

		CurValue = VRAMPtr[Index_x2];
		CurValue &= ~(Mask1 & 0xFF);
		CurValue |= (ColorComp1 & 0xFF);
		VRAMPtr[Index_x2] = CurValue;
	}
	else
	{
		// Line crosses 1 or more pixel boundries
		unsigned int ColorInside = ColorComposites[Color];
		unsigned int ShiftLeftAmount =  (1-(x2 & 0x01))<<2;

		// Set up our masking operation
		Mask1 = Mask1 >> ShiftRightAmount;
		ColorComp1 = ColorComp1 >> ShiftRightAmount;

		Mask2 = Mask2 << ShiftLeftAmount;
		ColorComp2 = ColorComp2 << ShiftLeftAmount;

		Mask1 &= Mask2;
		ColorComp1 &= ColorComp2;

		// Fill up the outside colors first
		// Take old colors out and put new colors in
		CurValue = VRAMPtr[Index_x1];
		CurValue &= ~(Mask1 >> 8);
		CurValue |= (ColorComp1 >> 8);
		VRAMPtr[Index_x1] = CurValue;

		CurValue = VRAMPtr[Index_x2];
		CurValue &= ~(Mask1 & 0xFF);
		CurValue |= (ColorComp1 & 0xFF);
		VRAMPtr[Index_x2] = CurValue;

		// Now loop through and fill up the Inside ones
		int i;
		for(i = 1; i < Index_dx; i++)
			VRAMPtr[Index_x1+i] = (ColorInside & 0xFF);
	}
}



// Draws a line using the bresenham's line algorithmn
void GFX_Line_2BPP(int x1, int y1, int x2, int y2, unsigned char Color, unsigned char *VRAMPtr)
{
	int dx, dy;
	int err, dx2, dy2, i, ix, iy;
	unsigned char CurValue;

	// difference between starting and ending points
	dx = x2 - x1;
	dy = y2 - y1;

	// calculate direction of the vector and store in ix and iy
	if (dx >= 0)
		ix = 1;

	if (dx < 0)
	{
		ix = -1;
		dx = abs(dx);
	}

	if (dy >= 0)
		iy = 1;

	if (dy < 0)
	{
		iy = -1;
		dy = abs(dy);
	}

	// scale deltas and store in dx2 and dy2
	dx2 = dx << 1;
	dy2 = dy << 1;

	if (dx > dy)	// dx is the major axis
	{
		// initialize the error term
		err = dy2 - dx;

		for (i = 0; i <= dx; i++)
		{
			// Plot the pixel
			// Calculate the shift amount to get to the individual pixel
			int ShiftAmount = 6 - (x1 & 0x03)*2;
			unsigned int Index = (g_ScreenWidth>>2)*y1 + (x1>>2);

			// We need to mask out the previous pixel
			CurValue = VRAMPtr[Index];
			CurValue &= ~(0x03 << ShiftAmount);
			CurValue |= (Color << ShiftAmount);
			VRAMPtr[Index] = CurValue;

			if (err >= 0)
			{
				err -= dx2;
				y1 += iy;
			}
			err += dy2;
			x1 += ix;
		}
	}

	else 		// dy is the major axis
	{
		// initialize the error term
		err = dx2 - dy;

		for (i = 0; i <= dy; i++)
		{
			// Plot the pixel
			// Calculate the shift amount to get to the individual pixel
			int ShiftAmount = 6 - (x1 & 0x03)*2;
			unsigned int Index = (g_ScreenWidth>>2)*y1 + (x1>>2);

			// We need to mask out the previous pixel
			CurValue = VRAMPtr[Index];
			CurValue &= ~(0x03 << ShiftAmount);
			CurValue |= (Color << ShiftAmount);
			VRAMPtr[Index] = CurValue;

			if (err >= 0)
			{
				err -= dy2;
				x1 += ix;
			}
			err += dx2;
			y1 += iy;
		}
	}
}



void GFX_Line_4BPP(int x1, int y1, int x2, int y2, unsigned char Color, unsigned char *VRAMPtr)
{
	int dx, dy;
	int err, dx2, dy2, i, ix, iy;
	unsigned char CurValue;

	// difference between starting and ending points
	dx = x2 - x1;
	dy = y2 - y1;

	// calculate direction of the vector and store in ix and iy
	if (dx >= 0)
		ix = 1;

	if (dx < 0)
	{
		ix = -1;
		dx = abs(dx);
	}

	if (dy >= 0)
		iy = 1;

	if (dy < 0)
	{
		iy = -1;
		dy = abs(dy);
	}

	// scale deltas and store in dx2 and dy2
	dx2 = dx << 1;
	dy2 = dy << 1;

	if (dx > dy)	// dx is the major axis
	{
		// initialize the error term
		err = dy2 - dx;

		for (i = 0; i <= dx; i++)
		{
			// Plot the pixel
			// Calculate the shift amount to get to the individual pixel
			int ShiftAmount = (x1 & 0x01) ? 0 : 4;
			unsigned int Index = (g_ScreenWidth>>1)*y1 + (x1>>1);

			// We need to mask out the previous pixel
			CurValue = VRAMPtr[Index];
			CurValue &= ~(0x0F << ShiftAmount);
			CurValue |= (Color << ShiftAmount);
			VRAMPtr[Index] = CurValue;

			if (err >= 0)
			{
				err -= dx2;
				y1 += iy;
			}
			err += dy2;
			x1 += ix;
		}
	}

	else 		// dy is the major axis
	{
		// initialize the error term
		err = dx2 - dy;

		for (i = 0; i <= dy; i++)
		{
			// Plot the pixel
			// Calculate the shift amount to get to the individual pixel
			int ShiftAmount = (x1 & 0x01) ? 0 : 4;
			unsigned int Index = (g_ScreenWidth>>1)*y1 + (x1>>1);

			// We need to mask out the previous pixel
			CurValue = VRAMPtr[Index];
			CurValue &= ~(0x0F << ShiftAmount);
			CurValue |= (Color << ShiftAmount);
			VRAMPtr[Index] = CurValue;

			if (err >= 0)
			{
				err -= dy2;
				x1 += ix;
			}
			err += dx2;
			y1 += iy;
		}
	}
}


// Draws a clipped line from (x1,y1) to (x2, y2)
void GFX_Line_Clip_2BPP(int x1, int y1, int x2, int y2, unsigned char Color, unsigned char *VRAMPtr)
{
	// Call the clipped line function first before calling our normal draw line function
	Clip_Line(&x1,&y1,&x2,&y2);
	GFX_Line_2BPP(x1,y1,x2,y2,Color,VRAMPtr);
}


void GFX_Line_Clip_4BPP(int x1, int y1, int x2, int y2, unsigned char Color, unsigned char *VRAMPtr)
{
	// Call the clipped line function first before calling our normal draw line function
	Clip_Line(&x1,&y1,&x2,&y2);
	GFX_Line_4BPP(x1,y1,x2,y2,Color,VRAMPtr);
}



// Taken right out of the Tricks of the Game Programming Windows Book
// Uses the Cohen-Sutherland Algorithmn
int Clip_Line(int *x1,int *y1,int *x2, int *y2)
{
	// this function clips the sent line using the globally defined clipping
	// region

	// internal clipping codes
	#define CLIP_CODE_C  0x0000
	#define CLIP_CODE_N  0x0008
	#define CLIP_CODE_S  0x0004
	#define CLIP_CODE_E  0x0002
	#define CLIP_CODE_W  0x0001

	#define CLIP_CODE_NE 0x000a
	#define CLIP_CODE_SE 0x0006
	#define CLIP_CODE_NW 0x0009
	#define CLIP_CODE_SW 0x0005

	int xc1=(*x1),
	    yc1=(*y1),
		xc2=(*x2),
		yc2=(*y2);

	int p1_code=0,
	    p2_code=0;

	// determine codes for p1 and p2
	if ((*y1) < g_ClipTop)
		p1_code|=CLIP_CODE_N;
	else
	if ((*y1) > g_ClipBottom)
		p1_code|=CLIP_CODE_S;

	if ((*x1) < g_ClipLeft)
		p1_code|=CLIP_CODE_W;
	else
	if ((*x1) > g_ClipRight)
		p1_code|=CLIP_CODE_E;

	if ((*y2) < g_ClipTop)
		p2_code|=CLIP_CODE_N;
	else
	if ((*y2) > g_ClipBottom)
		p2_code|=CLIP_CODE_S;

	if ((*x2) < g_ClipLeft)
		p2_code|=CLIP_CODE_W;
	else
	if ((*x2) > g_ClipRight)
		p2_code|=CLIP_CODE_E;

	// try and trivially reject
	if ((p1_code & p2_code))
		return(0);

	// test for totally visible, if so leave points untouched
	if (p1_code==0 && p2_code==0)
		return(1);

	// determine end clip point for p1
	switch(p1_code)
		  {
		  case CLIP_CODE_C: break;

		  case CLIP_CODE_N:
			   {
			   yc1 = g_ClipTop;
			   xc1 = (*x1) + 0.5+(g_ClipTop-(*y1))*((*x2)-(*x1))/((*y2)-(*y1));
			   } break;
		  case CLIP_CODE_S:
			   {
			   yc1 = g_ClipBottom;
			   xc1 = (*x1) + 0.5+(g_ClipBottom-(*y1))*((*x2)-(*x1))/((*y2)-(*y1));
			   } break;

		  case CLIP_CODE_W:
			   {
			   xc1 = g_ClipLeft;
			   yc1 = (*y1) + 0.5+(g_ClipLeft-(*x1))*((*y2)-(*y1))/((*x2)-(*x1));
			   } break;

		  case CLIP_CODE_E:
			   {
			   xc1 = g_ClipRight;
			   yc1 = (*y1) + 0.5+(g_ClipRight-(*x1))*((*y2)-(*y1))/((*x2)-(*x1));
			   } break;

		// these cases are more complex, must compute 2 intersections
		  case CLIP_CODE_NE:
			   {
			   // north hline intersection
			   yc1 = g_ClipTop;
			   xc1 = (*x1) + 0.5+(g_ClipTop-(*y1))*((*x2)-(*x1))/((*y2)-(*y1));

			   // test if intersection is valid, of so then done, else compute next
				if (xc1 < g_ClipLeft || xc1 > g_ClipRight)
					{
					// east vline intersection
					xc1 = g_ClipRight;
					yc1 = (*y1) + 0.5+(g_ClipRight-(*x1))*((*y2)-(*y1))/((*x2)-(*x1));
					} // end if

			   } break;

		  case CLIP_CODE_SE:
	      	   {
			   // south hline intersection
			   yc1 = g_ClipBottom;
			   xc1 = (*x1) + 0.5+(g_ClipBottom-(*y1))*((*x2)-(*x1))/((*y2)-(*y1));

			   // test if intersection is valid, of so then done, else compute next
			   if (xc1 < g_ClipLeft || xc1 > g_ClipRight)
			      {
				  // east vline intersection
				  xc1 = g_ClipRight;
				  yc1 = (*y1) + 0.5+(g_ClipRight-(*x1))*((*y2)-(*y1))/((*x2)-(*x1));
				  } // end if

			   } break;

		  case CLIP_CODE_NW:
	      	   {
			   // north hline intersection
			   yc1 = g_ClipTop;
			   xc1 = (*x1) + 0.5+(g_ClipTop-(*y1))*((*x2)-(*x1))/((*y2)-(*y1));

			   // test if intersection is valid, of so then done, else compute next
			   if (xc1 < g_ClipLeft || xc1 > g_ClipRight)
			      {
				  xc1 = g_ClipLeft;
			      yc1 = (*y1) + 0.5+(g_ClipLeft-(*x1))*((*y2)-(*y1))/((*x2)-(*x1));
				  } // end if

			   } break;

		  case CLIP_CODE_SW:
			   {
			   // south hline intersection
			   yc1 = g_ClipBottom;
			   xc1 = (*x1) + 0.5+(g_ClipBottom-(*y1))*((*x2)-(*x1))/((*y2)-(*y1));

			   // test if intersection is valid, of so then done, else compute next
			   if (xc1 < g_ClipLeft || xc1 > g_ClipRight)
			      {
				  xc1 = g_ClipLeft;
			      yc1 = (*y1) + 0.5+(g_ClipLeft-(*x1))*((*y2)-(*y1))/((*x2)-(*x1));
				  } // end if

			   } break;

		  default:break;

		  } // end switch

	// determine clip point for p2
	switch(p2_code)
		  {
		  case CLIP_CODE_C: break;

		  case CLIP_CODE_N:
			   {
			   yc2 = g_ClipTop;
			   xc2 = (*x2) + (g_ClipTop-(*y2))*((*x1)-(*x2))/((*y1)-(*y2));
			   } break;

		  case CLIP_CODE_S:
			   {
			   yc2 = g_ClipBottom;
			   xc2 = (*x2) + (g_ClipBottom-(*y2))*((*x1)-(*x2))/((*y1)-(*y2));
			   } break;

		  case CLIP_CODE_W:
			   {
			   xc2 = g_ClipLeft;
			   yc2 = (*y2) + (g_ClipLeft-(*x2))*((*y1)-(*y2))/((*x1)-(*x2));
			   } break;

		  case CLIP_CODE_E:
			   {
			   xc2 = g_ClipRight;
			   yc2 = (*y2) + (g_ClipRight-(*x2))*((*y1)-(*y2))/((*x1)-(*x2));
			   } break;

			// these cases are more complex, must compute 2 intersections
		  case CLIP_CODE_NE:
			   {
			   // north hline intersection
			   yc2 = g_ClipTop;
			   xc2 = (*x2) + 0.5+(g_ClipTop-(*y2))*((*x1)-(*x2))/((*y1)-(*y2));

			   // test if intersection is valid, of so then done, else compute next
				if (xc2 < g_ClipLeft || xc2 > g_ClipRight)
					{
					// east vline intersection
					xc2 = g_ClipRight;
					yc2 = (*y2) + 0.5+(g_ClipRight-(*x2))*((*y1)-(*y2))/((*x1)-(*x2));
					} // end if

			   } break;

		  case CLIP_CODE_SE:
	      	   {
			   // south hline intersection
			   yc2 = g_ClipBottom;
			   xc2 = (*x2) + 0.5+(g_ClipBottom-(*y2))*((*x1)-(*x2))/((*y1)-(*y2));

			   // test if intersection is valid, of so then done, else compute next
			   if (xc2 < g_ClipLeft || xc2 > g_ClipRight)
			      {
				  // east vline intersection
				  xc2 = g_ClipRight;
				  yc2 = (*y2) + 0.5+(g_ClipRight-(*x2))*((*y1)-(*y2))/((*x1)-(*x2));
				  } // end if

			   } break;

		  case CLIP_CODE_NW:
	      	   {
			   // north hline intersection
			   yc2 = g_ClipTop;
			   xc2 = (*x2) + 0.5+(g_ClipTop-(*y2))*((*x1)-(*x2))/((*y1)-(*y2));

			   // test if intersection is valid, of so then done, else compute next
			   if (xc2 < g_ClipLeft || xc2 > g_ClipRight)
			      {
				  xc2 = g_ClipLeft;
			      yc2 = (*y2) + 0.5+(g_ClipLeft-(*x2))*((*y1)-(*y2))/((*x1)-(*x2));
				  } // end if

			   } break;

		  case CLIP_CODE_SW:
			   {
			   // south hline intersection
			   yc2 = g_ClipBottom;
			   xc2 = (*x2) + 0.5+(g_ClipBottom-(*y2))*((*x1)-(*x2))/((*y1)-(*y2));

			   // test if intersection is valid, of so then done, else compute next
			   if (xc2 < g_ClipLeft || xc2 > g_ClipRight)
			      {
				  xc2 = g_ClipLeft;
			      yc2 = (*y2) + 0.5+(g_ClipLeft-(*x2))*((*y1)-(*y2))/((*x1)-(*x2));
				  } // end if

			   } break;

		  default:break;

		  } // end switch

	// do bounds check
	if ((xc1 < g_ClipLeft) || (xc1 > g_ClipRight) ||
		(yc1 < g_ClipTop) || (yc1 > g_ClipBottom) ||
		(xc2 < g_ClipLeft) || (xc2 > g_ClipRight) ||
		(yc2 < g_ClipTop) || (yc2 > g_ClipBottom) )
		{
		return(0);
		} // end if

	// store vars back
	(*x1) = xc1;
	(*y1) = yc1;
	(*x2) = xc2;
	(*y2) = yc2;

	return(1);

} // end Clip_Line


// Clears the screen with the color specified
void GFX_FillScreen_2BPP(unsigned char Color, unsigned char *VRAMPtr)
{
	static unsigned char ColorComposites[4] = { 0x00, 0x55, 0xAA, 0xFF };

	memset(VRAMPtr, ColorComposites[Color], (g_ScreenWidth>>2)*g_ScreenHeight);
}

// Clears the screen with the color specified
void GFX_FillScreen_4BPP(unsigned char Color, unsigned char *VRAMPtr)
{
	static unsigned char ColorComposites[16] = {
		0x00, 0x11, 0x22, 0x33,
		0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB,
		0xCC, 0xDD, 0xEE, 0xFF
	};

	memset(VRAMPtr, ColorComposites[Color], (g_ScreenWidth>>1)*g_ScreenHeight);
}


// Fills a rectangular region defined by x,y,(x+width),(y+height) (which will be clipped)
void GFX_FillRegion_2BPP(int x, int y, int Width, int Height, unsigned char Color, unsigned char *VRAMPtr)
{
	// Quick clip checking
	if(x > g_ClipRight) return;
	if(y > g_ClipBottom) return;

	if(x < g_ClipLeft) x = g_ClipLeft;
	if(y < g_ClipTop) y = g_ClipTop;

	if(x+Width > (g_ClipRight + 1)) Width = g_ClipRight - x + 1;
	if(y+Height > (g_ClipBottom + 1)) Height = g_ClipBottom - y + 1;

	// We will fill vertically
	int i,j;
	unsigned int ScreenPitch = g_ScreenWidth>>2;
	unsigned char CurValue;

	for(j=x; j < x+Width; j++)
	{
		// Calculate the shift amount to get to the individual pixel
		int ShiftAmount = 6 - (j & 0x03)*2;
		unsigned int Index = (g_ScreenWidth>>2)*y + (j>>2);

		unsigned char MaskValue = ~(0x03 << ShiftAmount);
		unsigned char ColorValue = (Color << ShiftAmount);

		// Now loop through and draw out all the pixels in the V-line
		for(i = y; i < y+Height; i++)
		{
			CurValue = VRAMPtr[Index];
			CurValue &= MaskValue;
			CurValue |= ColorValue;
			VRAMPtr[Index] = CurValue;
			Index += ScreenPitch;
		}
	}
}

// Fills a rectangular region defined by x,y,(x+width),(y+height) (which will be clipped)
void GFX_FillRegion_4BPP(int x, int y, int Width, int Height, unsigned char Color, unsigned char *VRAMPtr)
{
	// Quick clip checking
	if(x > g_ClipRight) return;
	if(y > g_ClipBottom) return;

	if(x < g_ClipLeft) x = g_ClipLeft;
	if(y < g_ClipTop) y = g_ClipTop;

	if(x+Width > (g_ClipRight + 1)) Width = g_ClipRight - x + 1;
	if(y+Height > (g_ClipBottom + 1)) Height = g_ClipBottom - y + 1;

	// We will fill vertically
	int i,j;
	unsigned int ScreenPitch = g_ScreenWidth>>1;
	unsigned char CurValue;

	for(j=x; j < x+Width; j++)
	{
		// Calculate the shift amount to get to the individual pixel
		int ShiftAmount = (j & 0x01) ? 0 : 4;
		unsigned int Index = (g_ScreenWidth>>1)*y + (j>>1);

		unsigned char MaskValue = ~(0x0F << ShiftAmount);
		unsigned char ColorValue = (Color << ShiftAmount);

		// Now loop through and draw out all the pixels in the V-line
		for(i=y; i < y+Height; i++)
		{
			CurValue = VRAMPtr[Index];
			CurValue &= MaskValue;
			CurValue |= ColorValue;
			VRAMPtr[Index] = CurValue;
			Index += ScreenPitch;
		}
	}
}



// Changes the composite saturation level (0->3, 0 being highest saturation)
void GFX_Composite_Sat(int Saturation)
{
	// Mask out old values
	PORTF &= ~(0x03);
	PORTF |= (Saturation & 0x03);
}




// Mid-point circle drawing algorithmn w/ Clipping
// http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
void GFX_Circle_2BPP(int x, int y, int radius, int color, unsigned char *VRAMPtr)
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int TempX = 0;
	int TempY = radius;

	GFX_Plot_Clip_2BPP(x, y + radius,color,VRAMPtr);
	GFX_Plot_Clip_2BPP(x, y - radius,color,VRAMPtr);
	GFX_Plot_Clip_2BPP(x + radius, y,color,VRAMPtr);
	GFX_Plot_Clip_2BPP(x - radius, y,color,VRAMPtr);

	while(TempX < TempY)
	{
		if(f >= 0)
		{
			TempY--;
			ddF_y += 2;
			f += ddF_y;
		}

		TempX++;
		ddF_x += 2;
		f += ddF_x;

		GFX_Plot_Clip_2BPP(x + TempX, y + TempY,color,VRAMPtr);
		GFX_Plot_Clip_2BPP(x - TempX, y + TempY,color,VRAMPtr);
		GFX_Plot_Clip_2BPP(x + TempX, y - TempY,color,VRAMPtr);
		GFX_Plot_Clip_2BPP(x - TempX, y - TempY,color,VRAMPtr);
		GFX_Plot_Clip_2BPP(x + TempY, y + TempX,color,VRAMPtr);
		GFX_Plot_Clip_2BPP(x - TempY, y + TempX,color,VRAMPtr);
		GFX_Plot_Clip_2BPP(x + TempY, y - TempX,color,VRAMPtr);
		GFX_Plot_Clip_2BPP(x - TempY, y - TempX,color,VRAMPtr);
	}
}



// Mid-point circle drawing algorithmn w/ Clipping
// http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
void GFX_Circle_4BPP(int x, int y, int radius, int color, unsigned char *VRAMPtr)
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int TempX = 0;
	int TempY = radius;

	GFX_Plot_Clip_4BPP(x, y + radius,color,VRAMPtr);
	GFX_Plot_Clip_4BPP(x, y - radius,color,VRAMPtr);
	GFX_Plot_Clip_4BPP(x + radius, y,color,VRAMPtr);
	GFX_Plot_Clip_4BPP(x - radius, y,color,VRAMPtr);

	while(TempX < TempY)
	{
		if(f >= 0)
		{
			TempY--;
			ddF_y += 2;
			f += ddF_y;
		}

		TempX++;
		ddF_x += 2;
		f += ddF_x;

		GFX_Plot_Clip_4BPP(x + TempX, y + TempY,color,VRAMPtr);
		GFX_Plot_Clip_4BPP(x - TempX, y + TempY,color,VRAMPtr);
		GFX_Plot_Clip_4BPP(x + TempX, y - TempY,color,VRAMPtr);
		GFX_Plot_Clip_4BPP(x - TempX, y - TempY,color,VRAMPtr);
		GFX_Plot_Clip_4BPP(x + TempY, y + TempX,color,VRAMPtr);
		GFX_Plot_Clip_4BPP(x - TempY, y + TempX,color,VRAMPtr);
		GFX_Plot_Clip_4BPP(x + TempY, y - TempX,color,VRAMPtr);
		GFX_Plot_Clip_4BPP(x - TempY, y - TempX,color,VRAMPtr);
	}
}


// Initializes parameters used for rendering font to the screen
void GFX_TMap_Size(int TileMapWidth, int TileMapHeight)
{
	g_TileMapWidth = TileMapWidth;
	g_TileMapHeight = TileMapHeight;
}



// This function prints a character to the next terminal position and updates the cursor position
// if the character is not  in the font data set that we support a space " " is printed.
void GFX_TMap_Print_Char(char ch)
{
	// test for control code first
	if (ch == 0x0A || ch == 0x0D)
	{
		// reset cursor to left side of screen
		g_Tmap_Cursor_X = 0;
		
		if (++g_Tmap_Cursor_Y >= g_TileMapHeight)
		{
			g_Tmap_Cursor_Y = g_TileMapHeight-1; 
			
			// scroll tile map data one line up vertically
			memcpy( g_TileMapBasePtr, g_TileMapBasePtr + g_TileMapWidth, g_TileMapWidth*(g_TileMapHeight-1) );
			
			// clear last line out 
			memset( g_TileMapBasePtr + g_TileMapWidth*(g_TileMapHeight-1), TMAP_BASE_BLANK, g_TileMapWidth );
		
		} // end if
		
		// return to caller
		return;
	
    } // end if
	else // normal printable character?
	{
		// convert to upper case first
		ch = toupper(ch);
		
		// now test if character is in range
		if (ch >= ' ' && ch <= '_') ch = ch - ' ' + TMAP_BASE_BLANK;
		else 
		ch = TMAP_BASE_BLANK;
		
		// write character to tile_map
		g_TileMapBasePtr[g_Tmap_Cursor_X + (g_Tmap_Cursor_Y * g_TileMapWidth) ] = ch;
		
		// update cursor position
		if (++g_Tmap_Cursor_X >= g_TileMapWidth)
		{
			g_Tmap_Cursor_X = 0;
			
			if (++g_Tmap_Cursor_Y >= g_TileMapHeight)
			{
				g_Tmap_Cursor_Y = g_TileMapHeight-1; 
				
				// scroll tile map data one line up vertically
				memcpy( g_TileMapBasePtr, g_TileMapBasePtr + g_TileMapWidth, g_TileMapWidth*(g_TileMapHeight-1) );
				
				// clear last line out 
				memset( g_TileMapBasePtr + g_TileMapWidth*(g_TileMapHeight-1), TMAP_BASE_BLANK, g_TileMapWidth );
			} // end if
		
		} // end if
		
		// return to caller
		return;
	
	} // end else

} // end GFX_TMap_Print_Char



// This function prints the sent string by making repeated calls to the character print function
void GFX_TMap_Print_String(char *string)
{
	while( *string != NULL)
	   GFX_TMap_Print_Char( *string++ );
} // end GFX_TMap_Print_String



// This function clears the tile map memory out with the sent absolute tile index
void GFX_TMap_CLS(char ch)
{
	// convert to upper case first
	ch = toupper(ch);
	
	// now test if character is in range
	if (ch >= ' ' && ch <= '_') ch = ch - ' ' + TMAP_BASE_BLANK;
	else 
	ch = TMAP_BASE_BLANK;

	// clear the memory
	memset( g_TileMapBasePtr, ch, g_TileMapWidth*g_TileMapHeight );
	
	// home cursor
	g_Tmap_Cursor_X = 0;
	g_Tmap_Cursor_Y = 0;
} // end GFX_TMap_CLS



// This function sets the cursor position for the tile mapped console
void GFX_TMap_SetCursor(int x, int y)
{
	// test if cursor is on screen
	if (x < 0) x = 0;
	else
	if (x >= g_TileMapWidth ) x = g_TileMapWidth - 1;
	
	if (y < 0) y = 0;
	else
	if (y >= g_TileMapHeight ) y = g_TileMapHeight - 1;
	
	// make assignment
	g_Tmap_Cursor_X = x;
	g_Tmap_Cursor_Y = y;
} // end GFX_TMap_Setcursor


