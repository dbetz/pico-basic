///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Copyright Nurve Networks LLC 2008
// 
// Filename: XGS_PIC_NTSC_TILE1_V010.H
//
// Original Author: Joshua Hintze
// 
// Last Modified: 9.7.08
//
// Description: Header file for XGS_PIC_NTSC_TILE1_V010.s assembly graphics driver. The purpose of this file
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
#ifndef XGS_PIC_NTSC_TILE1
#define XGS_PIC_NTSC_TILE1

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

#define SCREEN_TILE_MAP_WIDTH	20
#define SCREEN_TILE_MAP_HEIGHT	24
#define TILE_WIDTH				8
#define TILE_HEIGHT				8

#define SCREEN_WIDTH		160
#define SCREEN_HEIGHT		192
#define SCREEN_BPP			8

// Used for allocating buffers in main program
#define MAX_PALETTE_SIZE		256

#define VERTICAL_OFFSET		40
#define LINE_REPT_COUNT		1

#define START_VIDEO_SCAN    0
#define START_ACTIVE_SCAN   (VERTICAL_OFFSET)
#define END_ACTIVE_SCAN     (SCREEN_HEIGHT*LINE_REPT_COUNT + VERTICAL_OFFSET)
#define END_VIDEO_SCAN      261

#define MAX_SPRITES				6

// Sprite state controls
#define SPRITE_STATE_ON			0x01
#define SPRITE_STATE_OFF		0x00
#define SPRITE_STATE_ON_MASK	0x01

// Common NTSC colors
#define COLOR_LUMA		(0x91)

#define NTSC_GREEN  	(COLOR_LUMA + 0)        
#define NTSC_YELLOW   	(COLOR_LUMA + 2)        
#define NTSC_ORANGE   	(COLOR_LUMA + 4)        
#define NTSC_RED      	(COLOR_LUMA + 5)        
#define NTSC_VIOLET   	(COLOR_LUMA + 9)        
#define NTSC_PURPLE   	(COLOR_LUMA + 11)        
#define NTSC_BLUE     	(COLOR_LUMA + 13)        

#define NTSC_BLACK   	(0x4F)

#define NTSC_GRAY0    	(NTSC_BLACK + 0x10)        
#define NTSC_GRAY1    	(NTSC_BLACK + 0x20)        
#define NTSC_GRAY2    	(NTSC_BLACK + 0x30)        
#define NTSC_GRAY3    	(NTSC_BLACK + 0x40)        
#define NTSC_GRAY4    	(NTSC_BLACK + 0x50)        
#define NTSC_GRAY5    	(NTSC_BLACK + 0x60)        
#define NTSC_GRAY6    	(NTSC_BLACK + 0x70)        
#define NTSC_GRAY7    	(NTSC_BLACK + 0x80)        
#define NTSC_GRAY8    	(NTSC_BLACK + 0x90)        
#define NTSC_GRAY9    	(NTSC_BLACK + 0xA0)        
#define NTSC_WHITE    	(NTSC_BLACK + 0xB0) 



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


// read/write, logical width of tile map, usefull for making horizontal long maps larger than the 
// SCREEN_TILE_MAP_WIDTH. Typically this is set to SCREEN_TILE_MAP_WIDTH by default
volatile extern int g_TileMapLogicalWidth;   	

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// end multiple inclusions 
#endif
