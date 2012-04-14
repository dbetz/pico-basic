///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright Nurve Networks LLC 2008
//
// Filename: XGS_PIC_SYSTEM_V010.C
//
// Original Author: Joshua Hintze
//
// Last Modified: 8.31.08
//
// Description: System library file, it includes all our common types, macros, functions that are used
//				throughout the XGS PIC library.
//
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// First and foremost, include the chip definition header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// include local XGS API header files
#include "XGS_PIC_SYSTEM_V010.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHIP CONFIG SETTINGS - (FUSE BITS)  - MUST BE DEFINED ONLY ONCE IN CODE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_FICD(ICS_PGD2 & JTAGEN_OFF);		// Jtag on so we can use those pins, ISP on port 2
_FWDT(FWDTEN_OFF & WINDIS_OFF); 	// Disable Watchdog timer and windowed watchdog timer
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_EC) // Clock monitor: disabled switch: enabled, OSC2 is output, Using external clock
_FOSCSEL(FNOSC_FRC & IESO_OFF)		// Start with the fast rc until we switch to PLL, two speed oscillator startup disabled


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Global functional instruction rate. This is the instruction rate, not the clock rate. Fcy = 1/2 * Fclock
unsigned long g_FCY = FCY_NTSC;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Configures the clock PLL multiplier, this function will use the #define FCY above for the clock rate
void SYS_ConfigureClock(unsigned long FCY)
{
	// Save off the clcok rate
	g_FCY = FCY;

	// We need to initiate a clock switch. This is time critical code so we will use some special built-in
	// functions to help us out and unlocks/relocks these registers.
	__builtin_write_OSCCONH(0x07);	// Select the Fast RC clock with postscaler
	__builtin_write_OSCCONL(0x01);	// Initiate the switch by setting the switch enable bit

	// Wait for Clock Switch to occur
	while (OSCCONbits.COSC != 0b111);

	// To properly configure the PLL we need to switch between two clocks. First
	// start off with a Fast RC clock rate. Then modify the PLLPre,PLLPost,PLLDiv registers,
	// to set up the appropriate new clock rate. Then issue a clock switch.

	// Check for NTSC
	if(g_FCY == FCY_NTSC)
	{
		// Configures the clock to 24 * Input (NTSC) = 85.90908 MHz or Fcy = 42.95454 MHz
		// General Equation for PLL setup = Fosc = Fin ( M / (N1 * N2) )
		// M = PLLDIV = 96
		// N1 = PLLPRE = 2
		// N2 = PLLPOST = 2

		CLKDIVbits.PLLPRE = 0; 			// 0 = 2, 1 = 3, 2 = 4, etc
		CLKDIVbits.PLLPOST = 0; 		// 0 = 2
		PLLFBD = 94;					// PLL feedback divisor where 0 = 2, 1 = 3, etc
	}
	else // Must be VGA (In the future the user could add more speeds in here)
	{
		// Configures the clock to 3.5 * VGA dot clock = 88.1125 MHz or Fcy = 44.05625 MHz
		// Actual frequency is Fosc = 88.1463 MHz or Fcy = 44.0731 MHz so clock period is 22.69 nS
		// General Equation for PLL setup = Fosc = Fin ( M / (N1 * N2) )
		// M = PLLDIV = 197
		// N1 = PLLPRE = 4
		// N2 = PLLPOST = 2

		CLKDIVbits.PLLPRE = 2; 			// 0 = 2, 1 = 3, 2 = 4, etc
		CLKDIVbits.PLLPOST = 0; 		// 0 = 2
		PLLFBD = 195;					// PLL feedback divisor where 0 = 2, 1 = 3, etc
	}


	// We need to initiate a clock switch. This is time critical code so we will use some special built-in
	// functions to help us out and unlocks/relocks these registers.
	__builtin_write_OSCCONH(0x03);	// Select the External Clock with PLL
	__builtin_write_OSCCONL(0x01);	// Initiate the switch by setting the switch enable bit

	// Wait for Clock Switch to occur
	while (OSCCONbits.COSC != 0b011);

	// Wait for PLL to lock
	while (OSCCONbits.LOCK != 1);

}




