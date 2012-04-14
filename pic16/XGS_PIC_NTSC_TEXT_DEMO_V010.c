///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright Nurve Networks LLC 2008
//
// Filename: XGS_PIC_NTSC_TEXT_DEMO_V010.c
//
// Original Author: Joshua Hintze
//
// Last Modified: 11.28.08
//
// Description: Demonstrates simple usage of the text console functions available in the graphics API.
//
//
// Required Files: XGS_PIC_SYSTEM_V010.c/h
//				   XGS_PIC_GFX_DRV_V010.c/h
//				   XGS_PIC_NTSC_TILE2_V010.c/h
//				   XGS_PIC_TECH_FONT_6x8_V010.h
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Include main system header
#include "XGS_PIC_SYSTEM_V010.h"

// Include graphics drivers
#include "XGS_PIC_GFX_DRV_V010.h"
#include "XGS_PIC_NTSC_TILE2_V010.h"

// Include our font bitmaps
#include "XGS_PIC_TECH_FONT_6x8_V010.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The TILE map buffer must be allocated into far memory space because it is so large it wont fit into near
unsigned char g_TileMap[SCREEN_TILE_MAP_WIDTH*SCREEN_TILE_MAP_HEIGHT] __attribute__((far)); // 1 screens worth
unsigned char g_Palettes[MAX_PALETTE_SIZE]; 	// Different colors
Sprite g_Sprites[MAX_SPRITES];					// Max sprites



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Our main start location
int main (void)
{
	int Counter = 0, i;

	// Always call ConfigureClock first
	SYS_ConfigureClock(FCY_NTSC);

	// Init the graphics system
	GFX_InitTile(g_TileFontBitmap, g_TileMap, g_Palettes, g_Sprites);

	// Loop through and give everything their default color
	for(i = 0; i < MAX_PALETTE_SIZE; i++)
		g_Palettes[i] = i;

	// Set palette 0 to blue for text background, white for foreground
	g_Palettes[0] = NTSC_BLUE;
	g_Palettes[1] = NTSC_WHITE;

	// Initalize the Text Mode graphics functions
	GFX_TMap_Size(SCREEN_TILE_MAP_WIDTH, SCREEN_TILE_MAP_HEIGHT);

	// Clear out the text screen with spaces
	GFX_TMap_CLS(' ');

	// Start up our display interrupt
	GFX_StartDrawing(SCREEN_TYPE_NTSC);

	while(1)
	{
		// Wait until we are in v-sync to do any processing
		WAIT_FOR_VSYNC_START();

		// Slow down the screen update rate
		if(++Counter > 10)
		{
			Counter = 0;
			GFX_TMap_Print_String("XGS PIC 16-BIT               ");
		}

		// Wait here till we are done with v-sync
		WAIT_FOR_VSYNC_END();
		
	}

	return 0;
}


