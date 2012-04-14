///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Copyright Nurve Networks LLC 2008
// 
// Filename: XGS_PIC_NTSC_KEYBOARD_V010.c
//
// Original Author: Joshua Hintze
// 
// Last Modified: 9.28.08
//
// Description: Reads the Keyboard and displays the key's pressed on the TV using our
//				text mode fonts. Currently uses the Commodore 64 8x8 bit font.
//				
//				Controls:
//					Keyboard keys are displayed on the screen in a console text mode.
//					F5 - Clears the screen.
//
//
// Required Files: XGS_PIC_SYSTEM_V010.c/h
//				   XGS_PIC_KEYBOARD_DRV_V010.c/h
//				   XGS_PIC_GFX_DRV_V010.c/h
//				   XGS_PIC_NTSC_TILE1_V010.c/h
//				   XGS_PIC_C64_FONT_8x8_V010.h
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
#include "XGS_PIC_NTSC_TILE1_V010.h"

// Include our font bitmaps
#include "XGS_PIC_C64_FONT_8x8_V010.h"

// Include the keyboard driver
#include "XGS_PIC_KEYBOARD_DRV_V010.h"


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
	int i;

	// Always call ConfigureClock first
	SYS_ConfigureClock(FCY_NTSC);

	// Init the graphics system
	GFX_InitTile(g_TileFontBitmap, g_TileMap, g_Palettes, g_Sprites);

	// Keyboard init
	KEYBRD_Init();

	// Loop through and give everything their default color
	for(i = 0; i < MAX_PALETTE_SIZE; i++)
		g_Palettes[i] = i;

	// Set palette 0 to blue for text background, white for fon
	g_Palettes[0] = NTSC_BLUE;
	g_Palettes[1] = NTSC_WHITE;

	// Initalize the Text Mode graphics functions
	GFX_TMap_Size(SCREEN_TILE_MAP_WIDTH, SCREEN_TILE_MAP_HEIGHT);

	// Clear out the text screen with spaces
	GFX_TMap_CLS(' ');
	GFX_TMap_Print_String("XGS PIC 16BIT\nKEYBOARD DEMO V1.0\n\n");
	GFX_TMap_Print_String("TYPE ON THE KEYBOARD\n");
	GFX_TMap_Print_String("(F5 TO CLEAR SCREEN)\n\n");


	// Start up our display interrupt
	GFX_StartDrawing(SCREEN_TYPE_NTSC);

	while(1)
	{
		// Keyboard uninhibit before active region
		while(g_CurrentLine != 230);

		// Release keyboard clock line so it sends us data
		KEYBRD_Uninhibit();

		// Wait until we are in v-sync to do any processing
		WAIT_FOR_VSYNC_START();

		// Get the key press
		int Key;
		Key = KEYBRD_getch();

		// If there is a key draw it to the screen
		if(Key != 0)
			GFX_TMap_Print_Char(Key);

		// Check for F5 clear screen
		if(KEYBRD_IsKeyPressed(SCAN_F5))
		{
			GFX_TMap_CLS(' ');
			GFX_TMap_Print_String("XGS PIC 16BIT\nKEYBOARD DEMO V1.0\n\n");
			GFX_TMap_Print_String("TYPE ON THE KEYBOARD\n");
			GFX_TMap_Print_String("(F5 TO CLEAR SCREEN)\n\n");
		}


		// Wait here till we are done with v-sync
		WAIT_FOR_VSYNC_END();

		// Hold the clock line during active video
		KEYBRD_Inhibit();

	}

	return 0;
}


