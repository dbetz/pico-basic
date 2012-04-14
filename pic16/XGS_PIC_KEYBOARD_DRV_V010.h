///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright Nurve Networks LLC 2008
//
// Filename: XGS_PIC_KEYBOARD_V010.H
//
// Original Author: Joshua Hintze
//
// Last Modified: 8.31.08
//
// Description: Header file for XGS_PIC_KEYBOARD_V010.c
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
#ifndef XGS_PIC_KEYBOARD
#define XGS_PIC_KEYBOARD

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I/O pin map for Keyboard
//
// KEYBRD_CLK 	  	| RD10 | Pin 44 | Input
// KEYBRD_DATA 		| RD11 | Pin 45 | Input,Output
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// gamepad I/O declarations
#define KEYBRD_PORT				PORTDbits
#define KEYBRD_TRIS				TRISDbits

#define KEYBRD_DATA       		KEYBRD_PORT.RD11
#define KEYBRD_DATA_TRIS     	KEYBRD_TRIS.TRISD11

#define KEYBRD_CLK        		KEYBRD_PORT.RD10
#define KEYBRD_CLK_TRIS   		KEYBRD_TRIS.TRISD10


// Below is a big long list of keyboard scan code defines for use with IsKeyPressed() function
#define SCAN_A	0x1C
#define SCAN_B	0x32
#define SCAN_C	0x21
#define SCAN_D	0x23
#define SCAN_E	0x24
#define SCAN_F	0x2B
#define SCAN_G	0x34
#define SCAN_H	0x33
#define SCAN_I	0x43
#define SCAN_J	0x3B
#define SCAN_K	0x42
#define SCAN_L	0x4B
#define SCAN_M	0x3A
#define SCAN_N	0x31
#define SCAN_O	0x44
#define SCAN_P	0x4D
#define SCAN_Q	0x15
#define SCAN_R	0x2D
#define SCAN_S	0x1B
#define SCAN_T	0x2C
#define SCAN_U	0x3C
#define SCAN_V	0x2A
#define SCAN_W	0x1D
#define SCAN_X	0x22
#define SCAN_Y	0x35
#define SCAN_Z	0x1A

#define SCAN_0	0x45
#define SCAN_1	0x16
#define SCAN_2	0x1E
#define SCAN_3	0x26
#define SCAN_4	0x25
#define SCAN_5	0x2E
#define SCAN_6	0x36
#define SCAN_7	0x3D
#define SCAN_8	0x3E
#define SCAN_9	0x46

#define SCAN_GRAVE 		0x0E			// Grave is the (`) key
#define SCAN_MINUS		0x4E
#define SCAN_EQUALS		0x55
#define SCAN_BACKSLASH	0x5D			// (\) key
#define SCAN_BACKSPACE	0x66
#define SCAN_SPACE		0x29
#define SCAN_TAB		0x0D
#define SCAN_CAPS		0x58
#define SCAN_LSHIFT		0x12			// Left shift
#define SCAN_LCTRL		0x14			// Left control
#define SCAN_LALT		0x11
#define SCAN_RSHIFT		0x59
#define SCAN_RCTRL		0x94
#define SCAN_RALT		0x91			// Right ALT
#define SCAN_APPS		0xAF			// Application key (mouse/menu picture)
#define SCAN_ENTER		0x5A
#define SCAN_ESC		0x76			// Escape
#define SCAN_SCROLL		0x7E
#define SCAN_LBRACKET	0x54
#define SCAN_INSERT		0xF0
#define SCAN_HOME		0xEC
#define SCAN_PRIOR		0xFD			// Page Up
#define SCAN_PAGEUP		SCAN_PRIOR		// Simpler
#define SCAN_DELETE		0xF1
#define SCAN_END		0xE9
#define SCAN_NEXT		0xFA			// Page down
#define SCAN_PAGEDOWN	SCAN_NEXT		// Simpler
#define SCAN_UPARROW	0xF5
#define SCAN_LEFTARROW	0xEB
#define SCAN_DOWNARROW	0xF2
#define SCAN_RIGHTARROW	0xF4
#define SCAN_NUMLOCK	0x77
#define SCAN_NUMPADSLASH	0xCA
#define SCAN_NUMPADSTAR		0x7C
#define SCAN_NUMPADMINUS	0x7B
#define SCAN_NUMPADPLUS		0x79
#define SCAN_NUMPADENTER	0xD9
#define SCAN_NUMPADPERIOD	0x71
#define SCAN_NUMPAD0	0x70
#define SCAN_NUMPAD1	0x69
#define SCAN_NUMPAD2	0x72
#define SCAN_NUMPAD3	0x7A
#define SCAN_NUMPAD4	0x6B
#define SCAN_NUMPAD5	0x73
#define SCAN_NUMPAD6	0x74
#define SCAN_NUMPAD7	0x6C
#define SCAN_NUMPAD8	0x75
#define SCAN_NUMPAD9	0x7D
#define SCAN_RBRACKET	0x5B
#define SCAN_SEMICOLON	0x4C
#define SCAN_APOSTROPHE	0x52
#define SCAN_COMMA		0x41
#define SCAN_PERIOD		0x49
#define SCAN_SLASH		0x4A			// Forward slash

#define SCAN_F1		0x05
#define SCAN_F2		0x06
#define SCAN_F3		0x04
#define SCAN_F4		0x0C
#define SCAN_F5		0x03
#define SCAN_F6		0x0B
#define SCAN_F7		0x83
#define SCAN_F8		0x0A
#define SCAN_F9		0x01
#define SCAN_F10	0x09
#define SCAN_F11	0x78
#define SCAN_F12	0x07

// Number of key presses we'll store into our ascii fifo buffer
#define ASCII_FIFO_SIZE			32

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXTERNALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern volatile unsigned char g_KeyboardState[32];		// Keyboard state table (bit encoded) for a possible 256 defferent scan keys

// FIFO table and pointers
extern volatile unsigned char g_AsciiFifo[ASCII_FIFO_SIZE];
extern volatile int g_AsciiFifoInPtr;
extern volatile int g_AsciiFifoOutPtr;
extern volatile int g_AsciiFifoKeysInBuf;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void KEYBRD_Init();

// Functions for accessing the fifo
int KEYBRD_getch();
void KEYBRD_ungetch(unsigned char ch);

// Key state check
int KEYBRD_IsKeyPressed(unsigned char Key);


// The following two functions will stall the keyboard, telling it to buffer the keys. This is
// something you may want to do while drawing active video. The proper way of doing this is to
// mix your keyboard reading in with your video kernel, but it is more complex. Thus that is left
// to the user. You can use these functions and try to get a key during the vsync/blanking region
// but it may not work as well.

// Keyboard Inhibit Clock
void KEYBRD_Inhibit();

// Keyboard Uninhibit clock
void KEYBRD_Uninhibit();



// end multiple inclusions
#endif
