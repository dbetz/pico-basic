;/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ 
;/ Copyright Nurve Networks LLC 2008
;/ 
;/ Filename: XGS_PIC_VGA_TILE1_V010.S
;/ 
;/ Description: 176x240, 22x30, 8x8 tiles resolution VGA driver tile engine with 3 8x8 sprites
;/  
;/ Original Author: Joshua Hintze
;/ 
;/ Last Modified: 8.30.08
;/
;/ Overview:
;/   
;/ This is a 22x30 VGA tile engine. Each tile is 8x8 pixels, where each pixel is encoded as a full byte color index
;/ into the g_ColorTable. The reason for doing this is we can do color animation with the tiles and sprites. However the
;/ two most significant bits Bit 7:6 of the color table must remain zero since they are attached to the Hsync and 
;/ Vsync controls. This allows for 64 different colors.
;/
;/
;/ 22 tiles per row / 30 rows
;/
;/  colum 0      1       2              15       16       21
;/ |........|........|........| ... |........|........|........|  Row 0
;/ |........|........|........| ... |........|........|........|  Row 1
;/ |........|........|........| ... |........|........|........|  Row 2
;/ |........|........|........| ... |........|........|........|
;/ .
;/ .  
;/ .
;/ |........|........|........| ... |........|........|........|  Row 29
;/
;/ The engine supports fine scrolling in the vertical direction as well as course scrolling scrolling in the
;/ horizontal direction via a controllable tile map logical width or "pitch" parameter, so eventhough the physical
;/ screen is always 22x30 tiles the logical width can be anything up to 1024 tiles wide.
;/
;/ Additionally, the engine supports 3 8x8 pixel sprites with full 6-bit color. 
;/
;/ All sprites support scrolling off the right, left, and bottom of the screen. The sprite coordinates are slightly 
;/ translated relative to the tile coordinates, so sprite X=0, places the sprite just beyond the left edge of the screen,
;/ similary sprite X=183 places the sprite beyond the right edge of the screen. In other words, the sprite coordinates
;/ look like this:
;/
;/         <------- visible on screen --------->|, x's represent tile imagery
;/ 
;/         0                                    175        <--- tile pixels (22*8 = 0...176)
;/         |                                     | 
;/ --------xxxxxxxxxxxxxxxxxxxxxxx....xxxxxxxxxxxx--------
;/ |      |                                       | 
;/ 0      7                                       183      <--- sprite X coordinates (0..183)
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
;/  unsigned char State;        // Current State of the sprite (ex. On/Off)
;/  unsigned char Attribute;    // Attribute (not used in this engine)
;/  unsigned int X;             // X-position
;/  unsigned int Y;             // Y-position
;/  unsigned char *PixelData;   // Pointer to the sprite bitmap data 8x8
;/ } Sprite, *SpritePtr;
;/

;/ In most incarnations of the tile engines only x, y, and *bitmap are used. The remaining Attribute field is for the 
;/ caller's own use. Moreover, the field "state" is rarely used, and simply holds bit encoded on/off state of
;/ the sprite. Currently, the only state that is defined is:
;/
;/ #define SPRITE_ON        0x0001    // On bit
;/
;/ Other than that the only oher interesting field is the bitmap pointer *PixelData. This is a pointer that points
;/ to the 8x8 matrix of pixels that will be used for the sprite image. The data is in the SAME format as tiles, 
;/ thus you can point it to a tile, or another set of data as you wish. The important thing is that each sprite
;/ is 8x8 pixels where each pixel is a 6-bit color with bits 7:6 always equal to 0. Additionally, value 0 is used
;/ for transparent which equates to VGA black, however, the video will not turn black when this value is encoded,
;/ it directs the renderer not to draw anything, and thus makes the pixel transparent to the tile data under it.
;/
;/ Moving on, there are a few values that are EXPORTED out from the tile engine to give the user control of the 
;/ engine as well as indicate the state of the engine (what line its on, etc.), so the user can initiate render
;/ changes properly as not to disturb the video scan. These values are:
;/
;/ 
;/ .global _g_CurrentLine               ; read only, current physical line 0...524
;/ .global _g_RasterLine                ; read only, current logical raster line
;/ .global _g_VsyncFlag                 ; read only, Flag to indicate we are in v-sync
;/ .global _g_TileMapLogicalWidth       ; int, read/write, logical width of tile map
;/ .global _g_VscrollFine               ; int, read/write, current vertical scroll value 0..7
;/
;/ They are self explanatory. The caller can read g_CurrentLine to determine the video line 0..524 of the VGA scan,
;/ similarly _g_RasterLine indicates the logical raster line for the resolution. Finally, _g_TileMapLogicalWidth
;/ allows logical tile maps larger than the 25 wide to support scrolling, and _g_VscrollFile controls smooth 
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

;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ DEFINES
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

; width and height of screen 
.equ VGA_SCREEN_WIDTH,     176
.equ VGA_SCREEN_HEIGHT,    240

; controls the number of lines to shift the display vertically, later assign to global variable, so caller can control
.equ VGA_VERTICAL_OFFSET,  34

; number of bits each pixel is encoded with
.equ VGA_PIXELS_PER_BYTE,  1            

; Number of clocks per pixel (10 in the case of VGA) Modifying this won't really change anything,
; you will need to change code delays to get the correct clocks per pixel change
.equ CLOCKS_PER_PIXEL, 6 

; Number of lines we should repeat to reduce the vertical resolution
.equ LINE_REPT_COUNT,   2

; Mask to determine whether sprite bit is on
.equ SPRITE_ON_MASK,        0x0001

; If you want to add more variables to the sprite record make sure you increase
; the sprite struct size (in bytes) defined below
.equ SPRITE_STRUCT_SIZE, 8

; VGA Port bits for enabling and disabling syncs. These are used on PORTB only.
; -----------------------PORTB -------------------
;   7      6     5     4     3     2     1     0
; Vsync  Hsync   B1    B0    G1    G0    R1    R0
.equ VGA_HSYNC_BIT,     6
.equ VGA_VSYNC_BIT,     7


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
_g_CurrentLine: .int 0                  ; Line count 0->524, initalize higher than active so it resets its variables immediately
ReptLine:       .int LINE_REPT_COUNT    ; Repeated line counter 0->2
TILEINDPTR:     .int 0                  ; Current tile we are drawing
LINECOUNT:      .int 0                  ; Line counter increments by 8 bytes 0->56
_g_VsyncFlag:   .int 0                  ; Flag to indicate we are in v-sync
_g_VscrollFine: .int 0                  ; Fine scrolling flag 0->7
FILLBUFPTR:     .int 0                  ; Pointer to the buffer we want to fill
DRAWBUFPTR:     .int 0                  ; Pointer to the buffer we want to draw to the screen
DUMBYPORT:      .int 0                  ; Dumby port we will draw too during the first two lines since we don't have data queued up yet
_g_TileMapLogicalWidth: .int 22         ; Logical width of tile map
_g_RasterLine:   .int 0                 ; current logical raster line

; Uninitialized data sections
.section *,bss,near
ScanLineBuf0: .space (VGA_SCREEN_WIDTH + 2*8)   ; Scanline buffer is 22+2 tiles at 8 bytes = 216 bytes
ScanLineBuf1: .space (VGA_SCREEN_WIDTH + 2*8)   ; Scanline buffer is 25+2 tiles at 8 bytes = 216 bytes

;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ GLOBAL EXPORTS
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

.global __T1Interrupt               ; Declare Timer 1 ISR name global
.global _g_VsyncFlag                ; int, read, Flag to indicate we are in v-sync
.global _g_VscrollFine              ; int, read/write, Fine scrolling flag 0->7
.global _g_CurrentLine              ; int, read only, current physical line 0...524
.global _g_RasterLine               ; int, read only, current logical raster line
.global _g_TileMapLogicalWidth      ; int, read/write, logical width of tile map

;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
;/ ASM FUNCTIONS
;//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

; The following are code sections
.text           

__T1Interrupt:

        ; Our instruction clock is running at 2*VGA dot clock
        ; so every 2 clock cycles is a normal 640x480 pixel
        disi #0x3FFF                                ; (1) Disable interupts for up to 10000 cycles

        ; Save off variables we will use
        push.s                                      ; (1) Push shadow registers
        push W4                                     ; (1) Temp variable
        push W5                                     ; (1) Temp variable
        push W6                                     ; (1) Temp variable
        push W7                                     ; (1) Temp variable
        push W8                                     ; (1) Temp variable
        push W9                                     ; (1) Temp variable
        push RCOUNT                                 ; (1) Push rcount so we can use repeat loops

        ; Increment which line we are on
        inc _g_CurrentLine                          ; (1)
        mov _g_CurrentLine, W0                      ; (1) Set our line variable into W0

        ; Test if we are done with this frame (525 lines)
        mov #524, W2                                ; (1)
        cp W0, W2                                   ; (1) See if we are at line 524 = 0
        bra nz, DONE_LINE_TEST                      ; (1/2) if(_g_CurrentLine != 524) jump
        setm _g_CurrentLine                         ; (1) Reset Line counter set to 0xFFFF
DONE_LINE_TEST: 

        ; Total = 12

        ; The following section of code determines what kind of line we should be 
        ; drawing to the screen. Is it a Vsync Line, Black Top/Bottom, or an
        ; Active Video line. All of this is determined by the _g_CurrentLine counter
        ; variable

        ; if(_g_CurrentLine < VSYNC_LINE_COUNT)
        mov #1, W2                                  ; (1)
        cp W0, W2                                   ; (1) Compare to 1
        bra gt, NOT_VSYNC_LINE                      ; (1/2) if(Line > 1) jump

        ; Vsync line (17)
        repeat  #9                                  ; (1)
        nop                                         ; (1+n)
        bset LATB, #VGA_VSYNC_BIT                   ; (1) turn vsync bit on
        goto DRAW_BLACK_LINE                        ; (2) draw a black line to the screen

NOT_VSYNC_LINE:

        ; Test to see if this is a top blanking line
        mov #VGA_VERTICAL_OFFSET, W2                ; (1)
        cp W0, W2                                   ; (1) Compare to 33
        bra ge, NOT_TOP_BLANK_LINE                  ; (1/2) if(Line > 33) jump

        ; Blank Line Top (17)
        repeat  #2                                  ; (1)
        nop                                         ; (1+n)
        clr _g_RasterLine                           ; (1) reset rasterline count
        mov _g_TileMapBasePtr, W0                   ; (1) Copy Tile map start pointer to W0
        mov W0, TILEINDPTR                          ; (1) Set Tile Index Pointer
        bclr LATB, #VGA_VSYNC_BIT                   ; (1) turn vsync bit off
        goto DRAW_BLACK_LINE                        ; (2) draw a black line to the screen

NOT_TOP_BLANK_LINE:

        ; if(_g_CurrentLine > LAST_ACTIVE_LINE)
        mov #((VGA_SCREEN_HEIGHT*LINE_REPT_COUNT)+VGA_VERTICAL_OFFSET), W2 ; (1) Bottom line
        cp W0, W2                                   ; (1) Compare to 514
        bra lt, NOT_BOTTOM_BLANK_LINE               ; (1/2) if(Line < 514) jump

        ; Blank Line Bottom (17)
        nop                                         ; (1) delay
        nop                                         ; (1) delay
        bclr LATB, #VGA_VSYNC_BIT                   ; (1) turn vsync bit off
        setm _g_VsyncFlag                           ; (1) set the vsync flag 
        goto DRAW_BLACK_LINE                        ; (2) draw a black line to the screen

NOT_BOTTOM_BLANK_LINE:

        ; This must be an active line (17)
        bclr LATB, #VGA_VSYNC_BIT                   ; (1) turn vsync bit off
        clr _g_VsyncFlag                            ; (1) g_VsyncFlag = 0
        nop                                         ; (1) delay
        goto DRAW_ACTIVE_LINE                       ; (2) draw an active line to the screen



; ----------------- BLACK LINE -------------------------
DRAW_BLACK_LINE:

        ; This draws a black line with h-sync (used for v-sync and blanking lines)
        bset LATB, #VGA_HSYNC_BIT                   ; (1) turn hsync bit on

        ; Hold Hsync for 190 clocks total
        repeat #187                                 ; (1)
        nop                                         ; (1+n)

        bclr LATB, #VGA_HSYNC_BIT                   ; (1) turn hsync bit off and set to black RGB

        ; Done with this completely
        goto FINISHED_VGA                           ; Dont need to count because interrupts will keep us locked


; ----------------- ACTIVE LINE -------------------------
DRAW_ACTIVE_LINE:

        ; This draws an active line with h-sync, hsync is 190 clock cycles low
        ; ------- HSYNC ON ---------
        bset LATB, #VGA_HSYNC_BIT                   ; (1) turn hsync bit on

        ; Vertical fine scroll offsets first line - determine if we are on the first line
        mov _g_VscrollFine, W3                      ; (1) Copy vertical fine scrolling to W3, NOTE: this only is applicable on the first active scanline
        sl W3, #3, W3                               ; (1) Multiply fine scrolling by 8 since its going to sum into our byte offset

        mov _g_CurrentLine, W1                      ; (1) Compare W1 with line #34 = start of our active lines
        sub #35, W1                                 ; (1)
        bra le, FIRST_SECOND_LINE                   ; (1/2) branch if we are at the first line or second line
        mov #PORTB, W8                              ; (1)
        repeat #2                                   ; (1)
        nop                                         ; (1+n)
        goto DONE_FIRST_SECOND_LINE                 ; (2)
FIRST_SECOND_LINE:
        mov W3, LINECOUNT                           ; (1) Preset Line count
        mov #ScanLineBuf0, W0                       ; (1)
        mov W0, FILLBUFPTR                          ; (1) Start by filling scanlinebuf0
        mov #ScanLineBuf1, W0                       ; (1)
        mov W0, DRAWBUFPTR                          ; (1) Start by drawing out of scanlinebuf1
        mov #DUMBYPORT, W8                          ; (1) Render data to a dumby port on first two lines
DONE_FIRST_SECOND_LINE:

        ; Total = 13

        ; Assign the pointers to where we want to draw and fill
        mov FILLBUFPTR, W6                          ; (1) W6 is the fill pointer always
        mov DRAWBUFPTR, W7                          ; (1) W7 is the draw pointer always

        ; Figure out what type of scanline is this. Since we are line doubling
        ; we actually have two lines of data to fill up just one scanbuffer,
        ; while the second scanbuffer is rendering data out. Thus on even lines
        ; we will fill up the scanbuffer, on odd lines we will add sprites and
        ; other things.
        mov _g_CurrentLine, W1                      ; (1) Get the current line number in W1
        and #0x01, W1                               ; (1) Mask off bottom bit to see if we are odd
        bra z, EVEN_LINE_RENDER                     ; (1/2) If even jump
        goto ODD_LINE_RENDER                        ; (2) Jump to seperate rendering path on odd lines
EVEN_LINE_RENDER:

        ; Since the Fill and Draw buffers have a dead zone of 8 pixels to the left side we will skip past that
        ; dead zone by adding 8 to them. This makes it easier to draw sprites if we keep the dead zone.
        add #8, W6                                  ; (1) Increment past boarder
        add #8, W7                                  ; (1) Increment past boarder



        ; Start filling up the scan line buffer
        mov TILEINDPTR, W0                          ; (1) Get the tile index assigned to W0. This point to the memory location for the next tile index #
        mov LINECOUNT, W5                           ; (1) W5 equals our line counter into the tile, 0->56 incremented by 8
        mov _g_TileSet, W3                          ; (1) W3 points to the tile character data set
        mov _g_ColorTable, W4                       ; (1) Move the address of the color table to W4 so we can look up our colors and do color animation

        clr W2                                      ; (1) Clear out the top bits of W2 since we are going to be doing byte transfers to it

        ; Total after hsync = 26

        ; In this section we are setting W1 to be equal to the tile data for the first tile
        ; Also we are grabbing the first pixels color so we can render it right away
.rept 4 ; (Loop takes 37 clock cycles)
        ; Tile gathering setup
        clr W1                                      ; (1) Clear out the top bits of W1 since we are going to be doing byte transfers to it
        mov.b [W0++], W1                            ; (1) Get the tile index W1 = Index
        sl W1, #6, W1                               ; (1) W1 = W1 * 64, Index into tiles not bytes (tiles are 64 bytes long)
        add W3, W1, W1                              ; (1) W1 += TileSet pointer


        ; PIXEL 0
        mov.b [W1+W5], W2                           ; (2) 2 cycles because of pipeline stall. Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our final color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 1
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our final color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 2
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 3
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 4
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 5
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 6
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer

        ; PIXEL 7
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer

.endr

        ; ------- HSYNC OFF ---------
        bclr LATB, #VGA_HSYNC_BIT                   ; (1) turn hsync bit off

 
        ; ------ LEFT BLANKING REGION --------
        ; Now set PORTB rgb components to 0 (we should spend about 80 clock cycles here) 
        clr PORTB                                   ; (1) draw black

 
        ; Now during the left blanking lets write out 2 more tiles for a grand total of 6 out of 20 tiles
        ; the other tiles will need to be rendered during the actual pixel rendering of the other scanline buffer
.rept 2 ; (Loop takes 37 clock cycles)
        ; Tile gathering setup
        clr W1                                      ; (1) Clear out the top bits of W1 since we are going to be doing byte transfers to it
        mov.b [W0++], W1                            ; (1) Get the tile index W1 = Index
        sl W1, #6, W1                               ; (1) W1 = W1 * 64, Index into tiles not bytes (tiles are 64 bytes long)
        add W3, W1, W1                              ; (1) W1 += TileSet pointer


        ; PIXEL 0
        mov.b [W1+W5], W2                           ; (2) 2 cycles because of pipeline stall. Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our final color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 1
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our final color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 2
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 3
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 4
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 5
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 6
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 7
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer

.endr

        ; Fill up the remaining scanline buffer, we need 18 more tiles
        ; Also we need to start rendering out data so we'll do the same interleaved here
        ; Thus we will be drawing to the screen 13 tiles leaving 7 tiles remaining to be drawn
.rept 16 ; (Loop takes 37 clock cycles)
        ; Now draw out some pixels (6 clocks per pixel)
        ; Tile gathering setup
        clr W1                                      ; (1) Clear out the top bits of W1 since we are going to be doing byte transfers to it
        mov.b [W0++], W1                            ; (1) Get the tile index W1 = Index
        sl W1, #6, W1                               ; (1) W1 = W1 * 64, Index into tiles not bytes (tiles are 64 bytes long)
        add W3, W1, W1                              ; (1) W1 += TileSet pointer
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB


        ; PIXEL 0
        mov.b [W1+W5], W2                           ; (1) 2 cycles because of pipeline stall. Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our final color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 1
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our final color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 2
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 3
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 4
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB


        ; PIXEL 5
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer


        ; PIXEL 6
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer

        ; PIXEL 7
        mov.b [W1+W5], W2                           ; (1) Use indirect addressing to get assigned color, W5 = line offset
        add #1, W1                                  ; (1) Go to the next byte
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB
        mov.b [W4+W2], W2                           ; (1) Look up our color table value
        mov.b W2, [W6++]                            ; (1) Copy to scanline buffer
        nop                                         ; (1)
        nop                                         ; (1)
        nop                                         ; (1)
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB
        nop                                         ; (1)
.endr

        ; Finish off the remaining drawing
.rept 48
        ; Pixel
        nop                                         ; (1)
        nop                                         ; (1)
        nop                                         ; (1)
        nop                                         ; (1)
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB
        nop                                         ; (1)
.endr

        nop
        nop
        
FINISHED_RENDERING_LINE:
        ; Now set PORTB rgb components to 0
        mov #0xFFC0, W0                             ; (1) Load up our mask
        and LATB, WREG                              ; (1) mask off all color bits to 0
        mov WREG, PORTB                             ; (1) write out zeros

        ; Now increment our framepointer if we are done with this line (every 2 lines = 240 horizontal lines)
        dec ReptLine                                ; (1)
        bra nz, FINISHED_VGA                        ; (1/2)

        ; Increment our tile line count by 8 until we hit 64
        mov #8, W0                                  ; (1) 
        add LINECOUNT                               ; (1) Linecount += 8

        ; Increment our raster line count
        inc _g_RasterLine                           ; (1) g_RasterLine++

        ; Check to see if we hit 64
        mov #64, W0                                 ; (1)
        cp LINECOUNT                                ; (1)
        bra nz, NOT_NEW_TILE_ROW                    ; (1/2) if (Linecount != 64) jmp

        clr LINECOUNT                               ; (1) clear our tile line counter
        mov _g_TileMapLogicalWidth, W0              ; (1) W0 = Logical width of tile map
        add TILEINDPTR, WREG                        ; (1) W0 += TileIndexPointer
        mov W0, TILEINDPTR                          ; (1) save it out


NOT_NEW_TILE_ROW:

        ; Swap our filling and drawing buffers
        mov FILLBUFPTR, W0                          ; (1)
        mov DRAWBUFPTR, W1                          ; (1)
        mov W0, DRAWBUFPTR                          ; (1)
        mov W1, FILLBUFPTR                          ; (1) 

        mov #LINE_REPT_COUNT, W0                    ; (1) 
        mov W0, ReptLine                            ; (1) Reset our line repeate counter
        

        ; Done with this completely
FINISHED_VGA:

        BCLR IFS0, #T1IF                            ; Clear the Timer1 Interrupt flag Status
                                                    ; bit.

        pop RCOUNT
        pop W9
        pop W8
        pop W7
        pop W6
        pop W5
        pop W4                                      ; Retrieve context POP-ping from Stack
        pop.s                                       ; (1) Push status registers

        disi #0x0                                   ; Re-enable interrupts

        retfie                                      ;Return from Interrupt Service routine




; ------------------- ODD SCANLINE RENDERING PATH --------------------------------------
ODD_LINE_RENDER:

        ; Total clocks spent in hsync at this point is 22
        ; DO NOT USE OR TOUCH W7 or W8 in here!!!!
        
        ; -------- SPRITE RENDERING ---------------------
        ; Load Sprite list into W3      
        mov _g_SpriteList, W0                       ; (1) Sprite List head ptr
        mov _g_ColorTable, W9                       ; (1) Our color table

        ; Set up some comparision variables
        ; Use W2 as lower line bounds, W3 as upper line bound
        mov _g_CurrentLine, W2                      ; (1) Line count 0->525
        sub #34, W2                                 ; (1) Line count -= 34 because line 34 = start screen
        lsr W2, W2                                  ; (1) Line/2
        add #8, W2                                  ; (1) Line += 8 because 0 is off the screen on the top by a tile
        sub W2, #8, W3                              ; (1) Line - 8 = W3

        ; -------------------- SPRITE A ----------------------------
        ; DRAW SPRITE A to scanline buffer - takes 58 clocks

        ; Check to see if we are even drawing this sprite
        mov.b [W0], W1                              ; (1) Get sprite state
        and #SPRITE_ON_MASK, W1                     ; (1) Mask off on bit
        bra nz,SPRITE_ON_A                          ; (1/2) if sprite on branch

        ; Increment pointer to next sprite
        add #SPRITE_STRUCT_SIZE, W0                 ; (1)

        repeat #50                                  ; (1)
        nop                                         ; (1 + n)
        goto DONE_SPRITE_A                          ; (2)

SPRITE_ON_A:

        ; Get sprites X, Y positions and pixel data saved out
        mov [W0+2], W4                              ; (1) Get X position
        mov [W0+4], W1                              ; (1) Get Y position
        mov [W0+6], W5                              ; (1) Pixel Data pointer

        ; Increment pointer to next sprite
        add #SPRITE_STRUCT_SIZE, W0                 ; (1)

        ; if(Y < Line) check
        cp W2, W1                                   ; (1) Line - Y
        bra LT, NO_SPRITE_DRAW_A1                   ; (1/2) branch if less than line
        cp W3, W1                                   ; (1) Line + 8 - Y
        bra GEU, NO_SPRITE_DRAW_A2                  ; (1/2) branch if greater than line + 8
        
        ; Sprite is somewhere on this scan line
        sub W2, W1, W1                              ; (1) Find what line we are into the sprite
        sl W1, #3, W1                               ; (1) Multiply by 8
        add W5, W1, W5                              ; (1) Source = PixelData + SpriteY
        add W4, W6, W4                              ; (1) Destition = Scanlinebuf + SpriteX

.rept 8
        mov.b [W5++], W1                            ; (1) Get color index of pixel
        cp W1, #0                                   ; (1) Compare 0 and Color index to test for transparent
        bra z, 0f                                   ; (1/2) Determine if color is our transparent index = 0 if not branch
        mov.b [W9+W1], [W4]                         ; (1) Copy to scanline buffer since its not transparent
0:      
        add #1, W4                                  ; (1) increment our destination pointer
.endr
        goto DONE_SPRITE_A                          ; (2)

NO_SPRITE_DRAW_A1:
        nop                                         ; (1)
        nop                                         ; (1)
NO_SPRITE_DRAW_A2:
        repeat #43                                  ; (1)
        nop                                         ; (1+n)
DONE_SPRITE_A:      



        ; -------------------- SPRITE B ----------------------------
        ; DRAW SPRITE B to scanline buffer - takes 58 clocks

        ; Check to see if we are even drawing this sprite
        mov.b [W0], W1                              ; (1) Get sprite state
        and #SPRITE_ON_MASK, W1                     ; (1) Mask off on bit
        bra nz,SPRITE_ON_B                          ; (1/2) if sprite on branch

        ; Increment pointer to next sprite
        add #SPRITE_STRUCT_SIZE, W0                 ; (1)

        repeat #50                                  ; (1)
        nop                                         ; (1 + n)
        goto DONE_SPRITE_B                          ; (2)

SPRITE_ON_B:

        ; Get sprites X, Y positions and pixel data saved out
        mov [W0+2], W4                              ; (1) Get X position
        mov [W0+4], W1                              ; (1) Get Y position
        mov [W0+6], W5                              ; (1) Pixel Data pointer

        ; Increment pointer to next sprite
        add #SPRITE_STRUCT_SIZE, W0                 ; (1)

        ; if(Y < Line) check
        cp W2, W1                                   ; (1) Line - Y
        bra LT, NO_SPRITE_DRAW_B1                   ; (1/2) branch if less than line
        cp W3, W1                                   ; (1) Line + 8 - Y
        bra GEU, NO_SPRITE_DRAW_B2                  ; (1/2) branch if greater than line + 8
        
        ; Sprite is somewhere on this scan line
        sub W2, W1, W1                              ; (1) Find what line we are into the sprite
        sl W1, #3, W1                               ; (1) Multiply by 8
        add W5, W1, W5                              ; (1) Source = PixelData + SpriteY
        add W4, W6, W4                              ; (1) Destition = Scanlinebuf + SpriteX

.rept 8
        mov.b [W5++], W1                            ; (1) Get color index of pixel
        cp W1, #0                                   ; (1) Compare 0 and Color index to test for transparent
        bra z, 0f                                   ; (1/2) Determine if color is our transparent index = 0 if not branch
        mov.b [W9+W1], [W4]                         ; (1) Copy to scanline buffer since its not transparent
0:      
        add #1, W4                                  ; (1) increment our destination pointer
.endr
        goto DONE_SPRITE_B                          ; (2)

NO_SPRITE_DRAW_B1:
        nop                                         ; (1)
        nop                                         ; (1)
NO_SPRITE_DRAW_B2:
        repeat #43                                  ; (1)
        nop                                         ; (1+n)
DONE_SPRITE_B:      

        
        repeat #27                                  ; (1) Stall here for now to make even/odd scanline hsyncs the same
        nop                                         ; (1+n)

        ; Since the Fill and Draw buffers have a dead zone of 8 pixels to the left side we will skip past that
        ; dead zone by adding 8 to them. This makes it easier to draw sprites if we keep the dead zone.
        add #8, W6                                  ; (1) Increment past boarder
        add #8, W7                                  ; (1) Increment past boarder


        ; ------- HSYNC OFF ---------
        bclr LATB, #VGA_HSYNC_BIT                   ; (1) turn hsync bit off


        ; ------ LEFT BLANKING REGION --------
        ; Now set PORTB rgb components to 0 (we should spend about 80 clock cycles here) 
        clr PORTB                                   ; (1) draw black


        ; -------------------- SPRITE C ----------------------------
        ; DRAW SPRITE C to scanline buffer - takes 58 clocks

        ; Check to see if we are even drawing this sprite
        mov.b [W0], W1                              ; (1) Get sprite state
        and #SPRITE_ON_MASK, W1                     ; (1) Mask off on bit
        bra nz,SPRITE_ON_C                          ; (1/2) if sprite on branch

        ; Increment pointer to next sprite
        add #SPRITE_STRUCT_SIZE, W0                 ; (1)

        repeat #50                                  ; (1)
        nop                                         ; (1 + n)
        goto DONE_SPRITE_C                          ; (2)

SPRITE_ON_C:

        ; Get sprites X, Y positions and pixel data saved out
        mov [W0+2], W4                              ; (1) Get X position
        mov [W0+4], W1                              ; (1) Get Y position
        mov [W0+6], W5                              ; (1) Pixel Data pointer

        ; Increment pointer to next sprite
        add #SPRITE_STRUCT_SIZE, W0                 ; (1)

        ; if(Y < Line) check
        cp W2, W1                                   ; (1) Line - Y
        bra LT, NO_SPRITE_DRAW_C1                   ; (1/2) branch if less than line
        cp W3, W1                                   ; (1) Line + 8 - Y
        bra GEU, NO_SPRITE_DRAW_C2                  ; (1/2) branch if greater than line + 8
        
        ; Sprite is somewhere on this scan line
        sub W2, W1, W1                              ; (1) Find what line we are into the sprite
        sl W1, #3, W1                               ; (1) Multiply by 8
        add W5, W1, W5                              ; (1) Source = PixelData + SpriteY
        add W4, W6, W4                              ; (1) Destition = Scanlinebuf + SpriteX

.rept 8
        mov.b [W5++], W1                            ; (1) Get color index of pixel
        cp W1, #0                                   ; (1) Compare 0 and Color index to test for transparent
        bra z, 0f                                   ; (1/2) Determine if color is our transparent index = 0 if not branch
        mov.b [W9+W1], [W4]                         ; (1) Copy to scanline buffer since its not transparent
0:      
        add #1, W4                                  ; (1) increment our destination pointer
.endr
        goto DONE_SPRITE_C                          ; (2)

NO_SPRITE_DRAW_C1:
        nop                                         ; (1)
        nop                                         ; (1)
NO_SPRITE_DRAW_C2:
        repeat #43                                  ; (1)
        nop                                         ; (1+n)
DONE_SPRITE_C:

        repeat #14                                  ; (1)
        nop                                         ; (1+n)

        ; Render out the scanline
.rept VGA_SCREEN_WIDTH
        ; Pixel
        nop                                         ; (1)
        nop                                         ; (1)
        nop                                         ; (1)
        nop                                         ; (1)
        mov.b [W7++], [W8]                          ; (1) Copy out of the previously filled scanline buffer to PORTB
        nop                                         ; (1)
.endr

        goto FINISHED_RENDERING_LINE
        

;--------End of All Code Sections ---------------------------------------------

.end                               ;End of program code in this file
        