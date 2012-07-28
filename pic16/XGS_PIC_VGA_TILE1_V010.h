///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright Nurve Networks LLC 2008
//
// Filename: XGS_PIC_VGA_TILE1_V010.H
//
// Original Author: Joshua Hintze
//
// Last Modified: 9.7.08
//
// Description: Header file for XGS_PIC_VGA_TILE1_V010.s assembly graphics driver. The purpose of this file
//				is to define the different variations between this engine and the next. This makes it easier
//				for the person who is writting the main code and makes it less prone to error.
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
#ifndef XGS_PIC_VGA_TILE1
#define XGS_PIC_VGA_TILE1

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// simply interogate the video driver's g_CurrentLine variable and determine where the scanline is
#define VIDEO_ACTIVE(video_line)          ( ((video_line) >= START_ACTIVE_SCAN && (video_line) < END_ACTIVE_SCAN))
#define VIDEO_INACTIVE(video_line)        ( ((video_line) < START_ACTIVE_SCAN || (video_line) >= END_ACTIVE_SCAN))
#define VIDEO_TOP_OVERSCAN(video_line)    ( ((video_line) >= 0 && (video_line) < START_ACTIVE_SCAN))
#define VIDEO_BOT_OVERSCAN(video_line)    ( ((video_line) >= END_ACTIVE_SCAN))

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SCREEN_TILE_MAP_WIDTH	22
#define SCREEN_TILE_MAP_HEIGHT	30
#define TILE_WIDTH				8
#define TILE_HEIGHT				8

#define SCREEN_WIDTH		176
#define SCREEN_HEIGHT		240
#define SCREEN_BPP			8

#define VERTICAL_OFFSET		34
#define LINE_REPT_COUNT		2

#define START_VIDEO_SCAN    0
#define START_ACTIVE_SCAN   (VERTICAL_OFFSET)
#define END_ACTIVE_SCAN     (SCREEN_HEIGHT*LINE_REPT_COUNT + VERTICAL_OFFSET)
#define END_VIDEO_SCAN      524

// Used for allocating buffers in main program
#define MAX_PALETTE_SIZE		256

#define MAX_SPRITES				3

// Sprite state controls
#define SPRITE_STATE_ON			0x01
#define SPRITE_STATE_OFF		0x00
#define SPRITE_STATE_ON_MASK	0x01

// Common VGA colors
#define VGA_GREEN  		(0x0C)
#define VGA_YELLOW   	(0x0F)
#define VGA_ORANGE   	(0x0B)
#define VGA_RED      	(0x03)
#define VGA_VIOLET   	(0x33)
#define VGA_CYAN   		(0x3C)
#define VGA_BLUE     	(0x30)

#define VGA_BLACK   	(0x00)

#define VGA_GRAY0    	(0x15)
#define VGA_GRAY1    	(0x2A)
#define VGA_WHITE    	(0x3F)



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXTERNALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Values that are externed out of the tile engine
// read/write, Fine scrolling flag 0->7. For course scrolling use the g_TileMapBasePtr defined in the GFX lib and
// add SCREEN_TILE_MAP_WIDTH to scroll vertically by one tile.
volatile extern int g_VscrollFine;


// read/write, logical width of tile map, useful for making horizontal long maps larger than the
// SCREEN_TILE_MAP_WIDTH. Typically this is set to SCREEN_TILE_MAP_WIDTH by default
volatile extern int g_TileMapLogicalWidth;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// end multiple inclusions
#endif
