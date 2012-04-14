///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright Nurve Networks LLC 2008
//
// Filename: XGS_PIC_KEYBOARD_V010.c
//
// Original Author: Joshua Hintze
//
// Last Modified: 8.31.08
//
// Description: Keyboard library file
//
//
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


// include local XGS API header files
#include "XGS_PIC_SYSTEM_V010.h"
#include "XGS_PIC_KEYBOARD_DRV_V010.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// State machine for reading PS2 data from the keyboard
#define PS2_STATE_WAIT_START 	0
#define PS2_STATE_READ_DATA 	1
#define PS2_STATE_READ_PARITY 	2
#define PS2_STATE_READ_STOP 	3

#define PS2_DATA_MASK			0x800



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

volatile int g_PS2State = PS2_STATE_WAIT_START;
volatile int g_NumDataBitsRead = 0;
volatile unsigned char g_PS2ValueRead;
volatile int g_IsBreak = 0;						// Start of break code read?
volatile int g_IsShift = 0;						// Is shift key
volatile int g_IsExtended = 0;					// Extended character keys
volatile unsigned char g_KeyboardState[32];		// Keyboard state table (bit encoded) for a possible 256 defferent scan keys

// FIFO table and pointers
volatile unsigned char g_AsciiFifo[ASCII_FIFO_SIZE];
volatile int g_AsciiFifoInPtr = 0;
volatile int g_AsciiFifoOutPtr = 0;
volatile int g_AsciiFifoKeysInBuf = 0;

// Index is by scancode value, table data is ASCII
const unsigned char g_ScanCodeToAsciiUnshifted[128] = {
// Column    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F      // Row
 	       0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,9  ,'`',0  , 	//	0
 	       0  ,0  ,0  ,0  ,0  ,'q','1',0  ,0  ,0  ,'z','s','a','w','2',0  , 	//	1
 	       0  ,'c','x','d','e','4','3',0  ,0  ,32 ,'v','f','t','r','5',0  , 	//	2
 	       0  ,'n','b','h','g','y','6',0  ,0  ,0  ,'m','j','u','7','8',0  , 	//	3
 	       0  ,',','k','i','o','0','9',0  ,0  ,'.','/','l',';','p','-',0  , 	//	4
 	       0  ,0  ,39 ,0  ,'[','=',0  ,0  ,0  ,0  ,13 ,']',0  ,92 ,0  ,0  , 	//	5
 	       0  ,0  ,0  ,0  ,0  ,0  ,8  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  , 	//	6
 	       0  ,0  ,0  ,0  ,0  ,0  ,27 ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0    	//	7
};


const unsigned char g_ScanCodeToAsciiShifted[128] = {
// Column    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F      // Row
 	       0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,9  ,'~',0  , 	//	0
 	       0  ,0  ,0  ,0  ,0  ,'Q','!',0  ,0  ,0  ,'Z','S','A','W','@',0  , 	//	1
 	       0  ,'C','X','D','E','$','#',0  ,0  ,32 ,'V','F','T','R','%',0  , 	//	2
 	       0  ,'N','B','H','G','Y','^',0  ,0  ,0  ,'M','J','U','&','*',0  , 	//	3
 	       0  ,'<','K','I','O',')','(',0  ,0  ,'>','?','!',':','P','_',0  , 	//	4
 	       0  ,0  ,'"',0  ,'{','+',0  ,0  ,0  ,0  ,13 ,'}',0  ,'|',0  ,0  , 	//	5
 	       0  ,0  ,0  ,0  ,0  ,0  ,8  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  , 	//	6
 	       0  ,0  ,0  ,0  ,0  ,0  ,27 ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0    	//	7
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXTERNALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Interprets the scan code given
void PS2KeyDecode(unsigned char ScanCode);
unsigned char ScanToAscii(unsigned char ScanCode, int IsShift);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I/O pin map for game controllers
//
// KEYBRD_CLK 	  	| RD10 | Pin 44 | Input
// KEYBRD_DATA 		| RD11 | Pin 45 | Input,Output
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void KEYBRD_Init()
{
	// Set keyboard pins as inputs
	KEYBRD_DATA_TRIS = PIN_DIR_INPUT;
	KEYBRD_CLK_TRIS = PIN_DIR_INPUT;

	// Configure keyboard Interrupts
	INTCON2bits.INT3EP = 1;			// Falling edge triggered
	IEC3bits.INT3IE = 1;			// Enable interrupt on pin INT3
	IPC13bits.INT3IP = 6;			// Set priority level
	IFS3bits.INT3IF = 0;			// Clear interrupt flag

	// Clear out our state buffer
	memset((void*)g_KeyboardState,0,sizeof(g_KeyboardState));

} // end KEYBRD_Init


// Inhibits the keyboard from sending by setting the clock to an output and pulling the clock line low
void KEYBRD_Inhibit()
{
	// First disable the interrupt on the clock line
	IEC3bits.INT3IE = 0;			// Disable interrupt on pin INT3
	KEYBRD_CLK_TRIS = PIN_DIR_OUTPUT;
	KEYBRD_CLK = 0;					// Hold the clock low
}


// Releases the clock line and re-enables the interrupt
void KEYBRD_Uninhibit()
{
	// First disable the interrupt on the clock line
	KEYBRD_CLK_TRIS = PIN_DIR_INPUT;
	IEC3bits.INT3IE = 1;			// Enable interrupt on pin INT3
}



// Keyboard interrupt happens on the falling edge of the clock
void __attribute__((interrupt, no_auto_psv)) _INT3Interrupt(void)
{
	// TODO: Read current timer and compare against last time read, if its significant amount
	// then restart the state machine.

	// Immediately read the data line
	int DataState = (PORTD & PS2_DATA_MASK) ? 1 : 0;

	switch(g_PS2State)
	{
	case PS2_STATE_WAIT_START:
		if(DataState == 0) // Start bit is a 0
		{
			g_PS2State = PS2_STATE_READ_DATA;
			g_NumDataBitsRead = 0;
			g_PS2ValueRead = 0;
		}
	break;
	case PS2_STATE_READ_DATA:
		// Shift in LSB first
		g_PS2ValueRead = g_PS2ValueRead >> 1;
		g_PS2ValueRead |= (DataState ? 0x80 : 0);
		if(++g_NumDataBitsRead == 8)
			g_PS2State = PS2_STATE_READ_PARITY;
	break;
	case PS2_STATE_READ_PARITY:
		g_PS2State = PS2_STATE_READ_STOP;
	break;
	case PS2_STATE_READ_STOP:
		g_PS2State = PS2_STATE_WAIT_START;
		if(DataState == 1)
			PS2KeyDecode(g_PS2ValueRead);
	break;

	}

	IFS3bits.INT3IF = 0;			// Clear interrupt flag
}


inline unsigned char ScanToAscii(unsigned char ScanCode, int IsShift)
{
	if(IsShift)
		return g_ScanCodeToAsciiShifted[ScanCode];
	else
		return g_ScanCodeToAsciiUnshifted[ScanCode];
}


void PS2KeyDecode(unsigned char ScanCode)
{
	unsigned char Offset = 0;
	if(!g_IsBreak)
	{
		switch(ScanCode)
		{
		case 0xF0:				// Break Code identifier
			g_IsBreak = 1;
		break;
		case 0x12:				// Left shift
			g_IsShift = 1;
		break;
		case 0x59:				// Right shift
			g_IsShift = 1;
		break;
		case 0xE0:				// Extended keys
			g_IsExtended = 1;
		break;
		default:

			// Insert one into the state table at the correct offset
			if(g_IsExtended) Offset = 128;
			g_KeyboardState[(ScanCode >> 3) + Offset] = (1 << (ScanCode % 8));


			g_AsciiFifo[g_AsciiFifoInPtr] = ScanToAscii(ScanCode, g_IsShift);

			// Note: if this buffer overflows you will essentially lose all your previous keys
			g_AsciiFifoInPtr = (g_AsciiFifoInPtr + 1) % ASCII_FIFO_SIZE;

			// Done with this key, reset extended
			g_IsExtended = 0;
		break;
		}
	} // end if
	else
	{
		g_IsBreak = 0;
		switch(ScanCode)
		{
		case 0x12:				// Left SHIFT
			g_IsShift = 0;
		break;
		case 0x59:				// Right SHIFT
			g_IsShift = 0;
		break;
		default:
			// Keys released
			if(g_IsExtended) Offset = 128;
			g_KeyboardState[(ScanCode >> 3) + Offset] &= ~(1 << (ScanCode % 8));

			// Done with this key, reset extended
			g_IsExtended = 0;
		break;
		} // end switch
	} // end else
}



// This function gets a key (if available) from the keyboard Ascii fifio
int KEYBRD_getch()
{
	// Check for no data case
	if(g_AsciiFifoInPtr == g_AsciiFifoOutPtr)
		return 0;

	int AsciiValue = g_AsciiFifo[g_AsciiFifoOutPtr];
	g_AsciiFifoOutPtr = (g_AsciiFifoOutPtr + 1) % ASCII_FIFO_SIZE;

	return AsciiValue;
}


// This function places a key back onto the keyboard Ascii fifio
void KEYBRD_ungetch(unsigned char ch)
{
	// First we need to move our outpointer back
	g_AsciiFifoOutPtr--;

	// Check for underflow
	if(g_AsciiFifoOutPtr < 0)
		g_AsciiFifoOutPtr = ASCII_FIFO_SIZE;

	g_AsciiFifo[g_AsciiFifoOutPtr] = ch;

}

// Returns the keystate of the keyboard
int KEYBRD_IsKeyPressed(unsigned char Key)
{
	// Key state check
	return (g_KeyboardState[Key >> 3] & (1 << (Key % 8)));
}
