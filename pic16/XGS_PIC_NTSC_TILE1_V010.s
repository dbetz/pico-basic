;/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ 
;/ Copyright Nurve Networks LLC 2008
;/ 
;/ Filename: XGS_PIC_NTSC_TILE1_V010.S
;/ 
;/ Description: 160x192, 20x24, 8x8 tiles resolution NTSC driver tile engine with 6 8x8 sprites. 
;/  
;/ Original Author: Joshua Hintze
;/ 
;/ Last Modified: 8.30.08
;/
;/ Overview:
;/   
;/ This is a 20x24 NTSC tile engine. Each tile is 8x8 pixels, where each pixel is encoded as a full byte color index
;/ into the g_ColorTable. The reason for doing this is we can do color animation with the tiles and sprites. Each 
;/ pixel is encoded as a full byte LUMA4:CHROMA4. Note: sprites are still 8x8 pixels.
;/
;/ 20 tiles per row / 24 rows
;/
;/  colum 0      1       2              18       19       20
;/ |........|........|........| ... |........|........|........|  Row 0
;/ |........|........|........| ... |........|........|........|  Row 1
;/ |........|........|........| ... |........|........|........|  Row 2
;/ |........|........|........| ... |........|........|........|
;/ .
;/ .  
;/ .
;/ |........|........|........| ... |........|........|........|  Row 23
;/
;/ The engine supports fine scrolling in the vertical direction as well as course scrolling scrolling in the
;/ horizontal direction via a controllable tile map logical width or "pitch" parameter, so eventhough the physical
;/ screen is always 20x24 tiles the logical width can be anything up to 1024 tiles wide.
;/
;/ Additionally, the engine supports 6 8x8 pixel sprites with full NTSC color pixels. 
;/
;/ All sprites support scrolling off the right, left, and bottom of the screen. The sprite coordinates are slightly 
;/ translated relative to the tile coordinates, so sprite X=0, places the sprite just beyond the left edge of the screen,
;/ similary sprite X=168 places the sprite beyond the right edge of the screen. In other words, the sprite coordinates
;/ look like this:
;/
;/         <------- visible on screen --------->|, x's represent tile imagery
;/ 
;/         0                                    160        <--- tile pixels (20*8 = 0...160)
;/         |                                     | 
;/ --------xxxxxxxxxxxxxxxxxxxxxxx....xxxxxxxxxxxx--------
;/ |      |                                       | 
;/ 0      7                                       168      <--- sprite X coordinates (0..168)
;/
;/
;/ The interface to the tile engine from the C/C++ caller consists of a set of pointers to data structures as well as 
;/ some read/write variables exported from the tile engine that the caller needs to control the engines operation. Here 
;/ are the important variables that the tile engine must have initialized:
;/
;/ .extern _g_TileMapBasePtr    ; 16-bit pointer to tile map, allows indirection
;/ .extern _g_TileSet           ; 16-bit pointer to 8x8 tile bitmaps, allows indirection
;/ .extern _g_SpriteList        ; 16-bit pointer to sprite table records
;/ .extern _g_ColorTable        ; 16-bit pointer to a Color Table for pixel color indirection 
;/
;/ 
;/ First, are the tile map, bitmap, and sprite data structures. The tile map is a simple byte array MxN, where each
;/ byte represents the tile "index" to be rendered at that position. Each index refers to the tile bitmap that should
;/ be drawn. The tiles themselves are a continguous array of 8x8 pixel (byte) tiles, thus 64-bytes per tile. Both of
;/ these data structures are referred to by pointers to allow movement of the data structure in real-time. So the 
;/ caller would typically dynamically or statically allocate the tile map and tile bitmaps then point the below pointers
;/ at them, so the tile engine could locate them
;/
;/ Finally, there is the sprite table base address, this is a static name, thus the caller will typically define the 
;/ sprite_table as a static array of sprite records, where each record is a of the type:
;/
;/ // Sprite record structure
;/ typedef struct{
;/	unsigned char State;        // Current State of the sprite (ex. On/Off)
;/	unsigned char Attribute;    // Attribute (not used in this engine)
;/	unsigned int X;             // X-position
;/	unsigned int Y;             // Y-position
;/	unsigned char *PixelData;   // Pointer to the sprite bitmap data 8x8
;/ } Sprite, *SpritePtr;
;/

;/ In most incarnations of the tile engines only x, y, and *PixelData are used. The remaining Attribute field is for the 
;/ caller's own use. Moreover, the field "state" is rarely used, and simply holds bit encoded on/off state of
;/ the sprite. Currently, the only state that is defined is:
;/
;/ #define SPRITE_ON		0x0001    // On bit
;/
;/ Other than that the only oher interesting field is the bitmap pointer *PixelData. This is a pointer that points
;/ to the 8x8 matrix of pixels that will be used for the sprite image. The data is in the SAME format as tiles, 
;/ thus you can point it to a tile, or another set of data as you wish. The important thing is that each sprite
;/ is 8x8 pixels where each pixel is an NTSC byte. Additionally, value 0 is used for transparent which equates to
;/ NTSC black, however, the video will not turn black when this value is encoded, it directs the renderer not to
;/ draw anything, and thus makes the pixel transparent to the tile data under it.
;/
;/ Moving on, there are a few values that are EXPORTED out from the tile engine to give the user control of the 
;/ engine as well as indicate the state of the engine (what line its on, etc.), so the user can initiate render
;/ changes properly as not to disturb the video scan. These values are:
;/
;/ 
;/ .global _g_CurrentLine               ; read only, current physical line 0...261
;/ .global _g_RasterLine                ; read only, current logical raster line
;/ .global _g_VsyncFlag                 ; read only, Flag to indicate we are in v-sync
;/ .global _g_TileMapLogicalWidth       ; int, read/write, logical width of tile map
;/ .global _g_VscrollFine               ; int, read/write, current vertical scroll value 0..7
;/
;/ They are self explanatory. The caller can read g_CurrentLine to determine the video line 0..261 of the NTSC scan,
;/ similarly _g_RasterLine indicates the logical raster line for the resolution. Finally, _g_TileMapLogicalWidth
;/ allows logical tile maps larger than the 28 wide to support scrolling, and _g_VscrollFile controls smooth 
;/ scrolling vertically.
;/
;/ REMEMBER: When these global values are exported to C code. The leading "_" underscore is removed so you do not
;/           need to include the underscore.
;/ 
;/
;/ Modifications|Bugs|Additions:
;/
;/ Author Name: 
;/
;/
;/
;/
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ INCLUDES 
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

; Include our register and chip definitions file
.include "p24HJ256GP206.inc"

    .global __T1Interrupt   ; Declare Timer 1 ISR name global
	.global _g_VsyncFlag	; Flag to indicate we are in v-sync
	.global _g_VscrollFine	; Fine scrolling flag 0->7

;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ DEFINES
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

; width and height of screen 
.equ NTSC_SCREEN_WIDTH,     160
.equ NTSC_SCREEN_HEIGHT,    192

; controls the number of lines to shift the display vertically, later assign to global variable, so caller can control
.equ NTSC_VERTICAL_OFFSET,  40

; number of bits each pixel is encoded with
.equ NTSC_PIXELS_PER_BYTE,  1            

; Number of clocks per pixel (12 in the case of NTSC) Modifying this won't really change anything,
; you will need to change code delays to get the correct clocks per pixel change
.equ CLOCKS_PER_PIXEL, 12 

; Number of lines we should repeat to reduce the vertical resolution
.equ LINE_REPT_COUNT,   1

; Mask to determine whether sprite bit is on
.equ SPRITE_ON_MASK,		0x0001

; If you want to add more variables to the sprite record make sure you increase
; the sprite struct size (in bytes) defined below
.equ SPRITE_STRUCT_SIZE, 8

; Tile information
.equ NUM_HORZ_TILES,	20
.equ TILE_WIDTH,		8
.equ SCREEN_PITCH,		NUM_HORZ_TILES*TILE_WIDTH


; with the current D/A converter range 0 .. 15, 1 = 62mV
.equ NTSC_SYNC,           (0x0F)     ; 0V  
.equ NTSC_BLACK,          (0x4F)     ; 0.3V (about 30-40 IRE)

.equ NTSC_CBURST0,        (0x30) 
.equ NTSC_CBURST1,        (0x31) 
.equ NTSC_CBURST2,        (0x32) 

.equ NTSC_CHROMA_OFF, 	0x000F
.equ NTSC_OVERSCAN_CLR,	0x005F

.equ NTSC_GREEN,    (COLOR_LUMA + 0)        
.equ NTSC_YELLOW,   (COLOR_LUMA + 2)        
.equ NTSC_ORANGE,   (COLOR_LUMA + 4)        
.equ NTSC_RED,      (COLOR_LUMA + 5)        
.equ NTSC_VIOLET,   (COLOR_LUMA + 9)        
.equ NTSC_PURPLE,   (COLOR_LUMA + 11)        
.equ NTSC_BLUE,     (COLOR_LUMA + 14)        

.equ NTSC_GRAY0,    (NTSC_BLACK + 0x10)        
.equ NTSC_GRAY1,    (NTSC_BLACK + 0x20)        
.equ NTSC_GRAY2,    (NTSC_BLACK + 0x30)        
.equ NTSC_GRAY3,    (NTSC_BLACK + 0x40)        
.equ NTSC_GRAY4,    (NTSC_BLACK + 0x50)        
.equ NTSC_GRAY5,    (NTSC_BLACK + 0x60)        
.equ NTSC_GRAY6,    (NTSC_BLACK + 0x70)        
.equ NTSC_GRAY7,    (NTSC_BLACK + 0x80)        
.equ NTSC_GRAY8,    (NTSC_BLACK + 0x90)        
.equ NTSC_GRAY9,    (NTSC_BLACK + 0xA0)        
.equ NTSC_WHITE,    (NTSC_BLACK + 0xB0) 


;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ EXTERNALS
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

.extern _g_TileMapBasePtr       ; 16-bit pointer to tile map, allows indirection
.extern _g_SpriteList           ; 16-bit pointer to sprite table records
.extern _g_TileSet              ; 16-bit pointer to 8x8 tile bitmaps, allows indirection
.extern _g_ColorTable           ; 16-bit pointer to a Color Table for pixel color indirection 


;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ SRAM SECTIONS
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

; The following section describes initialized (data) near section. The reason we want
; to place these variables into the near SRAM area is so that ASM codes, that don't use many
; bits to encode data, will be able to access the variables.
.section *,data,near
_g_CurrentLine:		    .int 245		        ; Line count 0->261
_g_RasterLine:		    .int 0		            ; Line count 0->200
_g_TileMapLogicalWidth: .int NUM_HORZ_TILES     ; int, read/write, logical width of tile map, init to default size
REPTLINE:	            .int LINE_REPT_COUNT	; Repeated line counter 0->1
TILEINDPTR:	            .int 0			        ; Current tile we are drawing
LINECOUNT:	            .int 0			        ; Line counter increments by 6 bytes 0->48
_g_VsyncFlag:           .int 0		            ; Flag to indicate we are in v-sync
_g_VscrollFine:         .int 0		            ; Fine scrolling flag 0->7

; Uninitialized data sections
.section *,bss,near
SCAN_BUFFER:	.space SCREEN_PITCH+8			; Contains the scanline buffer for current line plus an extra 8 buffer to the left, filled in hsync
TILE_POINTERS:	.space (2*NUM_HORZ_TILES)		; Two bytes per tile

;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ GLOBAL EXPORTS
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

.global __T1Interrupt               ; Declare Timer 1 ISR name global
.global _g_VsyncFlag	            ; int, read, Flag to indicate we are in v-sync
.global _g_VscrollFine	            ; int, read/write, Fine scrolling flag 0->7
.global _g_CurrentLine              ; int, read only, current physical line 0...524
.global _g_RasterLine               ; int, read only, current logical raster line
.global _g_TileMapLogicalWidth      ; int, read/write, logical width of tile map
	

;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ MACROS
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

.macro Fill_Sprite

		; -------------------- SPRITE  ----------------------------
		; DRAW SPRITE  to scanline buffer - takes 58 clocks
		; Scanline buffer must be located in W6

		; Check to see if we are even drawing this sprite
		mov.b [W0], W1								; (1) Get sprite state
		and #SPRITE_ON_MASK, W1						; (1) Mask off on bit
		bra nz,0f									; (1/2) if sprite on branch

		; Increment pointer to next sprite
		add #SPRITE_STRUCT_SIZE, W0	    			; (1)

		repeat #50									; (1)
		nop											; (1 + n)
		goto 4f										; (2)

0:		; Sprite is turned on

		; Get sprites X, Y positions and pixel data saved out
		mov [W0+2], W4								; (1) Get X position
		mov [W0+4], W1								; (1) Get Y position
		mov [W0+6], W5								; (1) Pixel Data pointer

		; Increment pointer to next sprite
		add #SPRITE_STRUCT_SIZE, W0	    			; (1)

		; if(Y < Line) check
		cp W2, W1									; (1) Line - Y
		bra LT, 2f									; (1/2) branch if less than line
		cp W3, W1									; (1) Line + 8 - Y
		bra GEU, 3f									; (1/2) branch if greater than line + 8
		
		; Sprite is somewhere on this scan line
		sub W2, W1, W1								; (1) Find what line we are into the sprite
		sl W1, #3, W1								; (1) Multiply by 8
		add W5, W1, W5								; (1) Source = PixelData + SpriteY
		add W4, W6, W4								; (1) Destition = Scanlinebuf + SpriteX

.rept 8
		mov.b [W5++], W1							; (1) Get color index of pixel
		cp W1, #0									; (1) Compare 0 and Color index to test for transparent
		bra z, 1f									; (1/2) Determine if color is our transparent index = 0 if not branch
		mov.b [W9+W1], [W4]							; (1) Copy to scanline buffer since its not transparent
1:		; transparent color index (0)
		add #1, W4									; (1) increment our destination pointer
.endr
		goto 4f										; (2)

2: ; Done Sprite 1
		nop											; (1)
		nop											; (1)
3: ; Done Sprite 2
		repeat #43									; (1)
		nop											; (1+n)
4: ; Completely Done with sprite		

.endm




;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ ASM FUNCTIONS
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

; The following are code sections
.text           

__T1Interrupt:

		; Our instruction clock is running at 12*NTSC dot clock
		; so every 12 clock cycles is a normal 227 pixels
		disi #0x3FFF								; (1) Disable interupts for up to 10000 cycles

		; Save off variables we will use
		push.s										; (1) Push shadow registers
		push W4										; (1) Temp variable
		push W5										; (1) Temp variable
		push W6										; (1) Temp variable
		push W7										; (1) Temp variable
		push W8										; (1) Temp variable
		push W9										; (1) Temp variable
		push RCOUNT									; (1) Push rcount so we can use repeat loops

		; Increment which line we are on
		inc _g_CurrentLine									; (1)
		mov _g_CurrentLine, W0								; (1) Set our line variable into W0

		; Test if we are done with this frame (262 lines)
		mov #261, W2								; (1)
		cp W0, W2									; (1) See if we are at line 262 = 0
		bra nz, DONE_LINE_TEST						; (1/2) if(_g_CurrentLine != 262) jump
		setm _g_CurrentLine							; (1) Reset Line counter set to 0xFFFF
DONE_LINE_TEST: 

		; Total = 10



		; Test to see what kind of line type we have
		mov #5, W2									; (1)
		cp W0, W2									; (1) Compare to 5
		bra gt, NOT_VSYNC_LINE						; (1/2) if(Line > 5) jump

		; Vsync line (16)
		repeat	#9									; (1)
		nop											; (1+n)
		goto DRAW_VSYNC_LINE						; (2) draw a black line to the screen

NOT_VSYNC_LINE:

		; Test to see if this is a top blanking line
		mov #NTSC_VERTICAL_OFFSET, W2				; (1)
		cp W0, W2									; (1) Compare to NTSC_VERTICAL_OFFSET
		bra ge, NOT_TOP_OVERSCAN_LINE				; (1/2) if(Line >= NTSC_VERTICAL_OFFSET) jump

		; Blank Line Top (16)
		nop											; (1) delay
		nop											; (1) delay
		mov _g_TileMapBasePtr, W0					; (1) Copy Tile map start pointer to W0
		mov W0, TILEINDPTR							; (1) Set Tile Index Pointer
        clr _g_RasterLine                           ; (1) Reset raster line count
		mov #LINE_REPT_COUNT, W1					; (1) Reset repeat line back to count
		mov W1, REPTLINE							; (1)
		goto DRAW_OVERSCAN_LINE						; (2) draw a black line to the screen

NOT_TOP_OVERSCAN_LINE:

		; Test to see if this is a bottom blanking line
		mov #(NTSC_VERTICAL_OFFSET+NTSC_SCREEN_HEIGHT*LINE_REPT_COUNT), W2	; (1)
		cp W0, W2									; (1) Compare to 245
		bra lt, NOT_BOTTOM_OVERSCAN_LINE			; (1/2) if(Line < 245) jump

		; Blank Line Bottom (16)
		nop											; (1) delay
		nop											; (1) delay
		setm _g_VsyncFlag							; (1) set the vsync flag 
		goto DRAW_OVERSCAN_LINE						; (2) draw a black line to the screen

NOT_BOTTOM_OVERSCAN_LINE:

		; This must be an active line (16)
		clr _g_VsyncFlag							; (1) g_VsyncFlag = 0
		nop											; (1) delay
		goto DRAW_ACTIVE_LINE						; (2) draw an active line to the screen



; ----------------- VSYNC LINE -------------------------
DRAW_VSYNC_LINE:

		; This draws a vsync line with h-sync pulse inverted for serration
		; H-sync serration (4.7us) 236
		mov #NTSC_BLACK,W0 	                        ; (1) turn hsync on
		mov W0, LATB								; (1)
		repeat #232									; (1)
		nop											; (1+n)
		mov #NTSC_SYNC, W0	                        ; (1)
		mov W0, LATB								; (1)

		; Done with this completely
		goto FINISHED_NTSC							; Dont need to count because interrupts will keep us locked



; ----------------- OVERSCAN LINE -------------------------
DRAW_OVERSCAN_LINE:

		; This draws a overscan line with h-sync (used for blanking lines)
		; H-sync (4.7uS) = 202
		mov #NTSC_SYNC, W0	                        ; (1) turn hsync on
		mov W0, LATB								; (1)
		repeat #215									; (1) shorten it to reduce flagging
		nop											; (1+n)

		; Pre-burst (.6uS) = 26 clocks
		mov #NTSC_BLACK, W0	                        ; (1) turn hsync off black with no color signal
		mov W0, LATB								; (1)
		repeat #22									; (1)
		nop											; (1+n)


		; Color burst (10 NTSC clocks) = 120 clocks
		mov #NTSC_CBURST1, W0						; (1) turn hsync color burst on at phase 0
		mov W0, LATB								; (1)
		repeat #116									; (1)
		nop											; (1+n)


		; Post-burst (.6uS) = 26 clocks
		mov #NTSC_BLACK, W0	                        ; (1) turn hsync off black with no color signal
		mov W0, LATB								; (1)
		repeat #22									; (1)
		nop											; (1+n)


		; Overscan color
		mov #NTSC_OVERSCAN_CLR, W0					; (1) turn hsync off black with a color signal
		mov W0, LATB								; (1)

		; Done with this completely
		goto FINISHED_NTSC							; Dont need to count because interrupts will keep us locked




; ----------------- ACTIVE LINE -------------------------
DRAW_ACTIVE_LINE:


		; ------- HSYNC ON ---------
		; H-sync (4.7uS) = 202
		mov #NTSC_SYNC, W0 	                        ; (1) turn hsync on
		mov W0, LATB								; (1)


		; Determine if we are on the first line
		mov _g_CurrentLine, W1						; (1) Compare W1 with line start of our active lines
		sub #NTSC_VERTICAL_OFFSET, W1				; (1) Subtract for comparison
		bra z, FIRST_NTSC_LINE						; (1/2) branch if we are at the first line 
		nop											; (1)
		nop											; (1)

		goto DONE_FIRST_LINE						; (2)
FIRST_NTSC_LINE:
		; Vertical fine scroll offsets first line
		mov _g_VscrollFine, W3						; (1) Copy vertical fine scrolling to W3, NOTE: this only is applicable on the first active scanline
		sl W3, #3, W3								; (1) Multiply fine scrolling by 8 since its going to sum into our byte offset
		mov W3, LINECOUNT							; (1) Preset Line count
DONE_FIRST_LINE:

		; Total = 9

		; Assign the pointer to where we want to fill
		mov #SCAN_BUFFER, W6						; (1) W6 is the fill buffer always

		; Now we are going to start doing things a little backwards from the typical way of rendering to 
		; a scanline buffer. We are going to render out the sprites first to a cleared buffer. Then
		; during each pixel we are going to see if the buffer is already taken up by a sprite pixel or
		; not. If its not then the pixel drawn with be the tile map pixel.

		; The first thing we need to do is to clear out the scanline buffer (83 clocks)
		mov #0, W0									; (1) We'll be clearing it at 16-bit indexes so buffer must be even length
		repeat #((SCREEN_PITCH+8)/2)				; (1) Repeat this screen width / 2
		mov W0, [W6++]								; (1+n) Clear out 2 bytes		
		
		; Total = 93

		; Now render some sprites into the sprite buffer
		; -------- SPRITE RENDERING ---------------------
		; Load Sprite list into W0		
		mov _g_SpriteList, W0						; (1) Sprite List head ptr
		mov _g_ColorTable, W9						; (1) Our color table

		; Set up some comparision variables
		; Use W2 as lower line bounds, W3 as upper line bound
		mov _g_CurrentLine, W2						; (1) Line count 0->262
		sub #NTSC_VERTICAL_OFFSET, W2				; (1) Line count -= START_LINE because line START_LINE = start screen
		nop
		add #8, W2									; (1) Line += 8 because 0 is off the screen on the top by a tile
		sub W2, #8, W3								; (1) Line - 8 = W3

		; Reset to the beginning of the scanline buffer
		mov #SCAN_BUFFER, W6						; (1) W6 is the fill buffer always
		
		; Fill a sprite into the scanline buffer
		Fill_Sprite									; (58) Sprite 0
		Fill_Sprite									; (58) Sprite 1

		; ---------- PRE-BURST ----------------
		; Pre-burst (.6uS) = 30 clocks
		mov #NTSC_BLACK, W1	                        ; (1) turn hsync off black with no color signal
		mov W1, LATB								; (1)
;		mov _g_Breazeway1, W1						; (1)
;		repeat W1
;		nop
		repeat #22
		nop											; (1+n)


		; ------- COLORBURST ON ---------
		; Color burst (10 NTSC clocks) = 120 clocks
		mov #NTSC_CBURST1, W1 					    ; (1) turn hsync color burst on
		mov W1, LATB								; (1)


		; Time to fill a couple more sprites
		Fill_Sprite									; (58) Sprite 2
		Fill_Sprite									; (58) Sprite 3

		; Total = 118, fill to 120
		nop											; (1)	
		nop											; (1)	


		; -------- COLORBURST OFF ---------
		; FROM HERE ON it is imparative that we stay in sync with the color clock signal
		; (i.e. we don't change any pixels unless it is an integer multiple of 12)
		; If we don't do this then color changes will not be correct.
		; Post burst + Overscan = 223 clocks
		mov #NTSC_BLACK, W1	                        ; (1) turn hsync off black with no color signal
		mov W1, LATB								; (1)

		Fill_Sprite									; (58) Sprite 4
		Fill_Sprite									; (58) Sprite 5

		; Now prefill the tile pointer buffer so we don't have to waste time in the active scanline region
		mov TILEINDPTR, W0							; (1) Get the tile index assigned to W0. This point to the memory location for the next tile index #
		mov _g_TileSet, W3							; (1) W3 points to the tile character data set
		mov #TILE_POINTERS, W4						; (1) Get location to our tile pointers buffer

.rept NUM_HORZ_TILES
		clr W1										; (1) Clear out the top bits of W1 since we are going to be doing byte transfers to it
		mov.b [W0++], W1							; (1) Get the tile index W1 = Index
		sl W1, #6, W1								; (1) W1 = W1 * 64, Index into tiles not bytes (tiles are 8 bytes x 8 lines = 64 bytes long)
		add W3, W1, W1								; (1) W1 += TileSet start pointer
		mov W1, [W4++]								; (1) Copy over the tile pointer address
.endr



		; Get ready to draw tiles to the screen
		mov #TILE_POINTERS, W0						; (1) Get the tile index assigned to W0. This point to the memory location for the next tile index #
		mov LINECOUNT, W5							; (1) W5 equals our line counter into the tile, 0->56 incremented by 8
		mov _g_ColorTable, W4						; (1) Move the address of the color table to W4 so we can look up our colors and do color animation

		add #8, W6									; (1) Get W6 (scanline buffer) onto the start of the screen

		clr W7										; (1) Our zero comparision register
        clr W8                                      ; (1) Clear out the high bits of W8 since we fill only bottom bits

		; Total = 226, fill to 261-8
;		mov _g_Breazeway2, W1						; (1)
;		repeat W1									; (1)
;		nop											; (1+n)

		; -------- ACTIVE DRAWING REGION ---------
		; During the active region we will be looking up each pixels color index from the tile map
		; Then using that index to figure out the final color value. After we have that number we will
		; Look at the sprite scanline buffer. If there is nothing there we will use the tile color.
		; If the sprite value is filled then we will use the sprite color value

.rept NUM_HORZ_TILES ; (Loop takes XX clock cycles)

		; Tile gathering setup
		mov [W0++], W1								; (1) Get the tile index W1 = Index
		nop											; (1) Stop pipeline stall

		; PIXEL 0
		mov.b [W1+W5], W2							; (1) Use indirect addressing to get assigned color, W5 = line offset
		add #1, W1									; (1) Go to the next byte
		mov.b [W4+W2], W2							; (1) Look up our final color table value
		mov.b [W6++], W8							; (1) Get the sprite pixel value (0 means no pixel value)
		cpseq W7, W8								; (1/2) Check if our sprite scanline buffer is filled
		mov W8, W2  								; (1) Overwrite tile color with sprite color
		mov W2, PORTB								; (1) Output it
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay


		; PIXEL 1
		mov.b [W1+W5], W2							; (1) Use indirect addressing to get assigned color, W5 = line offset
		add #1, W1									; (1) Go to the next byte
		mov.b [W4+W2], W2							; (1) Look up our final color table value
		mov.b [W6++], W8							; (1) Get the sprite pixel value (0 means no pixel value)
		cpseq W7, W8								; (1/2) Check if our sprite scanline buffer is filled
		mov W8, W2  								; (1) Overwrite tile color with sprite color
		mov W2, PORTB								; (1) Output it
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay


		; PIXEL 2
		mov.b [W1+W5], W2							; (1) Use indirect addressing to get assigned color, W5 = line offset
		add #1, W1									; (1) Go to the next byte
		mov.b [W4+W2], W2							; (1) Look up our final color table value
		mov.b [W6++], W8							; (1) Get the sprite pixel value (0 means no pixel value)
		cpseq W7, W8								; (1/2) Check if our sprite scanline buffer is filled
		mov W8, W2  								; (1) Overwrite tile color with sprite color
		mov W2, PORTB								; (1) Output it
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay


		; PIXEL 3
		mov.b [W1+W5], W2							; (1) Use indirect addressing to get assigned color, W5 = line offset
		add #1, W1									; (1) Go to the next byte
		mov.b [W4+W2], W2							; (1) Look up our final color table value
		mov.b [W6++], W8							; (1) Get the sprite pixel value (0 means no pixel value)
		cpseq W7, W8								; (1/2) Check if our sprite scanline buffer is filled
		mov W8, W2  								; (1) Overwrite tile color with sprite color
		mov W2, PORTB								; (1) Output it
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay

		; PIXEL 4
		mov.b [W1+W5], W2							; (1) Use indirect addressing to get assigned color, W5 = line offset
		add #1, W1									; (1) Go to the next byte
		mov.b [W4+W2], W2							; (1) Look up our final color table value
		mov.b [W6++], W8							; (1) Get the sprite pixel value (0 means no pixel value)
		cpseq W7, W8								; (1/2) Check if our sprite scanline buffer is filled
		mov W8, W2  								; (1) Overwrite tile color with sprite color
		mov W2, PORTB								; (1) Output it
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay

		; PIXEL 5
		mov.b [W1+W5], W2							; (1) Use indirect addressing to get assigned color, W5 = line offset
		add #1, W1									; (1) Go to the next byte
		mov.b [W4+W2], W2							; (1) Look up our final color table value
		mov.b [W6++], W8							; (1) Get the sprite pixel value (0 means no pixel value)
		cpseq W7, W8								; (1/2) Check if our sprite scanline buffer is filled
		mov W8, W2  								; (1) Overwrite tile color with sprite color
		mov W2, PORTB								; (1) Output it
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay

		; PIXEL 6
		mov.b [W1+W5], W2							; (1) Use indirect addressing to get assigned color, W5 = line offset
		add #1, W1									; (1) Go to the next byte
		mov.b [W4+W2], W2							; (1) Look up our final color table value
		mov.b [W6++], W8							; (1) Get the sprite pixel value (0 means no pixel value)
		cpseq W7, W8								; (1/2) Check if our sprite scanline buffer is filled
		mov W8, W2  								; (1) Overwrite tile color with sprite color
		mov W2, PORTB								; (1) Output it
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay

		; PIXEL 7
		mov.b [W1+W5], W2							; (1) Use indirect addressing to get assigned color, W5 = line offset
		add #1, W1									; (1) Go to the next byte
		mov.b [W4+W2], W2							; (1) Look up our final color table value
		mov.b [W6++], W8							; (1) Get the sprite pixel value (0 means no pixel value)
		cpseq W7, W8								; (1/2) Check if our sprite scanline buffer is filled
		mov W8, W2  								; (1) Overwrite tile color with sprite color
		mov W2, PORTB								; (1) Output it
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay

.endr

		; Finish off this pixel before we go to black overscan
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay
		nop											; (1) delay


		
FINISHED_RENDERING_LINE:
		; Now set PORTB rgb components to 0
		mov #(NTSC_OVERSCAN_CLR | NTSC_OVERSCAN_CLR), W0 ; (1) turn hsync off black with no color signal
		mov W0, PORTB								; (1) Output it

		; Now increment our framepointer if we are done with this line (every 2 lines = 240 horizontal lines)
		dec REPTLINE								; (1)
		bra nz, FINISHED_NTSC						; (1/2)

        ; Increment our Raster line counter
        inc _g_RasterLine                           ; (1)

		; Increment our tile line count by 8 until we hit 64
		mov #8, W0									; (1) 
		add LINECOUNT								; (1) Linecount += 8

		; Check to see if we hit 64
		mov #64, W0									; (1)
		cp LINECOUNT								; (1)
		bra nz, NOT_NEW_TILE_ROW					; (1/2) if (Linecount != 64) jmp

		clr LINECOUNT								; (1) clear our tile line counter
		mov _g_TileMapLogicalWidth, W0				; (1) Number of tiles in increment 
		add TILEINDPTR, WREG						; (1) W0 += TileIndexPointer
		mov W0, TILEINDPTR							; (1) save it out


NOT_NEW_TILE_ROW:

		mov #LINE_REPT_COUNT, W0					; (1) 
		mov W0, REPTLINE							; (1) Reset our line repeate counter
		

		; Done with this completely
FINISHED_NTSC:

        bclr IFS0, #T1IF           					; Clear the Timer1 Interrupt flag Status
                                   					; bit.

		pop RCOUNT
		pop W9
		pop W8
		pop W7
		pop W6
		pop W5
		pop W4                   					; Retrieve context POP-ping from Stack
		pop.s										; (1) Push status registers

		disi #0x0									; Re-enable interrupts

        retfie                     					;Return from Interrupt Service routine




;--------End of All Code Sections ---------------------------------------------

.end                               ;End of program code in this file
        