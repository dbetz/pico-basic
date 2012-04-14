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
// Description: Header file for XGS_PIC_GFX_DRV_V010.c. This file contains functions for controlling and configuring
//				the graphics engine. Remember to include the appropriate graphics assembly file.
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// watch for multiple inclusions
#ifndef XGS_PIC_GFX_DVR
#define XGS_PIC_GFX_DVR

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Wait for Vsync macros
#define WAIT_FOR_VSYNC_START()  while(!g_VsyncFlag);
#define WAIT_FOR_VSYNC_END()	while(g_VsyncFlag);

// Types used for GFX_StartDrawing function (used primarily to setup the timer interrupt period
#define SCREEN_TYPE_NTSC		0
#define SCREEN_TYPE_VGA			1


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// STRUCTS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The following is the sprite structure
typedef struct{
	unsigned char State;			// State of the Sprite (SPRITE_STATE_ON/OFF)
	unsigned char Attribute;		// Attribute of the sprite (NOT USED)
	unsigned int X;					// X position of the sprite
	unsigned int Y;					// Y position of the sprite
	unsigned char *PixelData;		// Pointer to the bitmap data of the sprite
} Sprite, *SpritePtr;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXTERNALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

volatile extern unsigned int g_CurrentLine;   	// Current physical line 0...524(VGA),261(NTSC)
volatile extern int g_RasterLine;           	// Current logical raster line
volatile extern int g_VsyncFlag;				// Flag to indicate whether we are in v-sync or not

extern unsigned char *g_TileMapBasePtr;			// Used to do coarse scrolling in a tile map engine
extern unsigned char *g_TileSet; 				// 16-bit pointer to a Character set as 8(6)x8 with 1 byte per pixel
extern Sprite *g_SpriteList;					// 16-bit pointer to sprite table records

extern int g_TileMapWidth;						// Used to store the tile map width for text console modes
extern int g_TileMapHeight;						// Used to store the tile map height for text console modes
extern int g_Tmap_Cursor_X;						// Text mode cursor x position
extern int g_Tmap_Cursor_Y;						// Text mode cursor y position

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Initializes the graphics engine with bitmap parameters
void GFX_InitBitmap(int ScreenWidth, int ScreenHeight, int ScreenBPP, unsigned char *VRAMPtr,
			  		unsigned char *Palettes, unsigned short *PaletteMap);

// Initializes the graphics engine with tilemap parameters
void GFX_InitTile(unsigned char *TileData, unsigned char *TileMap, unsigned char *Palettes, Sprite* SpriteTable);

// Starts the interrupt for drawing to the screen
void GFX_StartDrawing(int ScreenType);

// Sets the region all clipped functions will be clipped against
void GFX_SetClipRegion(int Top, int Bottom, int Left, int Right);

// Returns the color of a pixel. Useful for collision detections
unsigned char GFX_GetPixel_2BPP(int x, int y, unsigned char *VRAMPtr);
unsigned char GFX_GetPixel_4BPP(int x, int y, unsigned char *VRAMPtr);

// Plots a single pixel non-clipped to the viewport
void GFX_Plot_2BPP(int x, int y, unsigned char Color, unsigned char *VRAMPtr);
void GFX_Plot_4BPP(int x, int y, unsigned char Color, unsigned char *VRAMPtr);

// Plots a single pixel clipped to the viewport
void GFX_Plot_Clip_2BPP(int x, int y, unsigned char Color, unsigned char *VRAMPtr);
void GFX_Plot_Clip_4BPP(int x, int y, unsigned char Color, unsigned char *VRAMPtr);

// Draws a non-clipped line from (x1,y1) to (x2, y2)
void GFX_Line_2BPP(int x1, int y1, int x2, int y2, unsigned char Color, unsigned char *VRAMPtr);
void GFX_Line_4BPP(int x1, int y1, int x2, int y2, unsigned char Color, unsigned char *VRAMPtr);

// Draws a clipped line from (x1,y1) to (x2, y2)
void GFX_Line_Clip_2BPP(int x1, int y1, int x2, int y2, unsigned char Color, unsigned char *VRAMPtr);
void GFX_Line_Clip_4BPP(int x1, int y1, int x2, int y2, unsigned char Color, unsigned char *VRAMPtr);

// Draws a clipped horizontal line from (x1) to (x2)
void GFX_HLine_2BPP(int x1, int x2, int y, unsigned char Color, unsigned char *VRAMPtr);
void GFX_HLine_4BPP(int x1, int x2, int y, unsigned char Color, unsigned char *VRAMPtr);

// Draws a clipped vertical line from (y1) to (y2)
void GFX_VLine_2BPP(int x, int y1, int y2, unsigned char Color, unsigned char *VRAMPtr);
void GFX_VLine_4BPP(int x, int y1, int y2, unsigned char Color, unsigned char *VRAMPtr);

// Clears the screen with the color specified
void GFX_FillScreen_2BPP(unsigned char Color, unsigned char *VRAMPtr);
void GFX_FillScreen_4BPP(unsigned char Color, unsigned char *VRAMPtr);

// Fills a rectangular region defined by x,y,(x+width),(y+width) (which will be clipped)
void GFX_FillRegion_2BPP(int x, int y, int Width, int Height, unsigned char Color, unsigned char *VRAMPtr);
void GFX_FillRegion_4BPP(int x, int y, int Width, int Height, unsigned char Color, unsigned char *VRAMPtr);

// Circle drawing routines
void GFX_Circle_2BPP(int x, int y, int radius, int color, unsigned char *VRAMPtr);
void GFX_Circle_4BPP(int x, int y, int radius, int color, unsigned char *VRAMPtr);

// Changes the composite saturation level (0->3, 0 being highest saturation)
void GFX_Composite_Sat(int Saturation);

//////// Tile map text mode functions ////////
// Initializes the TMap size so we can properly scroll text
void GFX_TMap_Size(int TileMapWidth, int TileMapHeight);

// Prints a single character to the tile map
void GFX_TMap_Print_Char(char ch);

// Prints a NULL-terminated string to the text mode console
void GFX_TMap_Print_String(char *string);

// Fills the screen of the tile map text mode with ch
void GFX_TMap_CLS(char ch);

// Sets the cursor on the tile map display
void GFX_TMap_SetCursor(int x, int y);

// end multiple inclusions
#endif
