///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright Nurve Networks LLC 2008
//
// Filename: XGS_PIC_UART_DRV_V010.c
//
// Original Author: Joshua Hintze
//
// Last Modified: 8.31.08
//
// Description: UART library file
//
//
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// include local XGS API header files
#include "XGS_PIC_SYSTEM_V010.h"
#include "XGS_PIC_UART_DRV_V010.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEFINES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Macros for enabling/disabling interrupts
#define Uart_disable_it_tx() 	(IEC0bits.U1TXIE = 0)
#define Uart_disable_it_rx() 	(IEC0bits.U1RXIE = 0)
#define Uart_enable_it_tx() 	(IEC0bits.U1TXIE = 1)
#define Uart_enable_it_rx() 	(IEC0bits.U1RXIE = 1)

#define Uart_tx_ready() 		(U1STAbits.UTXBF != 1)
#define Uart_rx_ready()			(U1STAbits.URXDA == 1)
#define Uart_send_byte(ch)      (U1TXREG=ch)
#define Uart_get_byte()         (U1RXREG)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

volatile unsigned char UART1_tx_buffer[UART1_TX_BUFFER_SIZE];
volatile unsigned int UART1_tx_buffer_add_pointer;
volatile unsigned int UART1_tx_buffer_send_pointer;
volatile unsigned int UART1_tx_buffer_bytes_to_send;

volatile unsigned char UART1_rx_buffer[UART1_RX_BUFFER_SIZE];
volatile unsigned int UART1_rx_buffer_add_pointer;
volatile unsigned int UART1_rx_buffer_out_pointer;
volatile unsigned int UART1_rx_buffer_bytes_in_buffer;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXTERNALS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I/O pin map for uart
//
// UART_TX 	  | RF3 | Pin 33 | Output
// UART_RX 	  | RF2 | Pin 34 | Input
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Initializes UART1 to the baud rate specified in config.h
void UART_Init(unsigned long BaudRate)
{
	// initialize the ring buffers
	UART1_tx_buffer_add_pointer = 0;
	UART1_tx_buffer_send_pointer = 0;
	UART1_tx_buffer_bytes_to_send = 0;
	UART1_rx_buffer_add_pointer = 0;
	UART1_rx_buffer_out_pointer = 0;
	UART1_rx_buffer_bytes_in_buffer = 0;
	
	// Configure interrupts
    // clear IF flags
    IFS0bits.U1RXIF = 0;
    IFS0bits.U1TXIF = 0;

    // set priority...must be set higher than or equal to cpu priority
    IPC2bits.U1RXIP = 4;
    IPC3bits.U1TXIP = 4;

    // enable interrupts
    IEC0bits.U1RXIE = 1;
    IEC0bits.U1TXIE = 1;

	// Now since we are overclocking, we can't really use the datasheet formula to come up with a good baud
	// rate calculation. So we will used the formula to get a starting point and then try it out on a couple
	// programs to get the final rates for our NTSC and VGA clock periods

	// With that being said, here is the general formula for U1BRG, we will use BRGH = 1.
	// Baud rate calculation is UxBRG = g_FCY/(16*Baud) - 1 using BRGH = 0
	// Baud rate calculation is UxBRG = g_FCY/(4*Baud) - 1 using BRGH = 1

	if(g_FCY == FCY_NTSC)
	{
		switch(BaudRate)
		{
		case 9600: U1BRG = 1147; break;
		case 19200: U1BRG = 574; break;
		case 38400: U1BRG = 287; break;
		case 57600: U1BRG = 191; break;
		case 115200: U1BRG = 95; break;
		default: U1BRG = (unsigned int)(((float)g_FCY/(4*BaudRate))+.5) - 1; break;	// We will use BRGH = 1 for more resolution, round
		}
	}
	else if(g_FCY == FCY_VGA)
	{
		switch(BaudRate)
		{
		case 9600: U1BRG = 1178; break;
		case 19200: U1BRG = 588; break;
		case 38400: U1BRG = 294; break;
		case 57600: U1BRG = 196; break;
		case 115200: U1BRG = 97; break;
		default: U1BRG = (unsigned int)(((float)g_FCY/(4*BaudRate))+.5) - 1; break;	// We will use BRGH = 1 for more resolution, round
		}
	}
	else
		U1BRG = (unsigned int)(((float)g_FCY/(4*BaudRate))+.5) - 1; 		// We will use BRGH = 1 for more resolution, round

    U1MODE = 0x8008; 				// 8 bits, no parity, uart enabled, BRGH = 1
    U1STA |= 0x0400; 				// Enable uart tx pin
}

////////////////////////////////////////////////////////////////////////////
// INTERUPTS
////////////////////////////////////////////////////////////////////////////

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
	IFS0bits.U1RXIF = 0;

	while (Uart_rx_ready()) {

    	if(UART1_rx_buffer_bytes_in_buffer == UART1_RX_BUFFER_SIZE)  //no space available
    	{
    		// We just dropped a byte!!!
    	    UART1_rx_buffer_out_pointer=(UART1_rx_buffer_out_pointer+1)%UART1_RX_BUFFER_SIZE; 
    	    UART1_rx_buffer_bytes_in_buffer--;
    	}
    
    	UART1_rx_buffer[UART1_rx_buffer_add_pointer]=Uart_get_byte();
    	UART1_rx_buffer_add_pointer=(UART1_rx_buffer_add_pointer+1)%UART1_RX_BUFFER_SIZE;
    	UART1_rx_buffer_bytes_in_buffer++;
    }   
}

void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void)
{
	IFS0bits.U1TXIF = 0;

	if(UART1_tx_buffer_bytes_to_send==0)
	{
		Uart_disable_it_tx(); //disable interrupt - nothing to send
	}
	else if(Uart_tx_ready())
	{
		Uart_send_byte(UART1_tx_buffer[UART1_tx_buffer_send_pointer]);
		UART1_tx_buffer_send_pointer=(UART1_tx_buffer_send_pointer+1)%UART1_TX_BUFFER_SIZE;
		UART1_tx_buffer_bytes_to_send--;
	}
}

////////////////////////////////////////////////////////////////////////////

int UART_putchar (int ch)
{
	Uart_disable_it_tx();

#ifdef STALL_UART
	while(UART1_tx_buffer_bytes_to_send == UART1_TX_BUFFER_SIZE)
	{
		// Enable the interrupt and just wait until the UART clears - this will STALL the program
		Uart_enable_it_tx();
	}
#else
	if(UART1_tx_buffer_bytes_to_send == UART1_TX_BUFFER_SIZE)  //check space available
	{
		// We are out of space in our transmit buffer
		Uart_enable_it_tx();
		return 0;
	}
#endif

	UART1_tx_buffer[UART1_tx_buffer_add_pointer]=ch;
	UART1_tx_buffer_add_pointer=(UART1_tx_buffer_add_pointer+1)%UART1_TX_BUFFER_SIZE;
	UART1_tx_buffer_bytes_to_send++;
	_U1TXInterrupt();

	Uart_enable_it_tx();
	return 1;
}

int UART_getchar(void)
{
  Uart_disable_it_rx();
  if(UART1_rx_buffer_bytes_in_buffer==0)
  {
	  Uart_enable_it_rx();
	  return -1;
  }
  else
  {
	  int ch = UART1_rx_buffer[UART1_rx_buffer_out_pointer];
	  UART1_rx_buffer_out_pointer=(UART1_rx_buffer_out_pointer+1)%UART1_RX_BUFFER_SIZE;
	  UART1_rx_buffer_bytes_in_buffer--;
	  Uart_enable_it_rx();
	  return ch;
  }
}
