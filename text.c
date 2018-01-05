/*	
 *	@author		abradbury
 *	
 *	Text.c handles the receiving and sending of text and RTTTL messages.
 */

#include "lpc17xx_can.h"
#include "canbus_msg.h"
#include "serial.h"	
#include "can.h"
#include "stdlib.h"
#include "debug_frmwrk.h"
#include "string.h"
#include "lcd.h"
#include "music.h"
#include "morse.h"
#include "mysys.h"
#include "text.h"
#include "menu.h"

#define TSTART	0x18009440
#define TEXT	0x18006440
#define TEND	0x1800A440

#define MSTART	0x1C009440
#define RTTTL	0x1C006440
#define MEND	0x1C00A440

uint8_t 		*dataArray;		// Pointer to the dataArray
int				count;			// Holds the number of blocks
int 			i;		
extern int		morseEnable;	// A flag, 1 if morse is enables, 0 otherwise
int				rtttl= 0;		// RTTTL flag
CAN_MSG_Type	Msg;			// Stores the message to be sent

/*	
 *	init_text() is called when a start message block is received. It gets the 
 *	number of text blocks that will follow, from the block count part of the 
 *	start message header, and creates an array to store the expected data.
 *	
 *	@param	msg			The start block received
 */
void init_text(CAN_MSG_Type msg)
{
	count = CAN_GET_COUNT(msg.id);
	i = 0;
	if(count == 0)
	{
		write_usb_serial_blocking("Error! Block count is 0\n\r",27);
	}
	else
	{
		UARTPutDec16((LPC_UART_TypeDef *)LPC_UART0, count);
		dataArray = MSYS_Alloc(sizeof(*dataArray) *(count*8));
	}
}

/*	
 *	rx_text() deals with received RTTTL and text messages. For text messages 
 *	it iterates over the 2 4-element arrays in each text block and stores the 
 *	contained values into the main dataArray array.
 *	
 *	@param	msg			The text block received
 *	@return	dataArray	The array where the received data is stored
 */
uint8_t* rx_text(CAN_MSG_Type msg)
{	
	int j=0,k=0;
	for(j=0; j<4; j++)
	{
		dataArray[(8*i)+j] = msg.dataA[j];
	}

	for(k=0; k<4; k++)
	{
		dataArray[(8*i)+(k+4)] = msg.dataB[k];
	}

	i++;
	
	return dataArray;
}

/*	
 *	tx_text() receives a string and destination address and composes the blocks 
 *	needed to send the string as either a text or RTTTL message over the network.
 *	Further detail is given by inline comments.
 *	
 *	@param	str			The string to send
 *	@param	to			The number of the station to send to
 *	@param	type		't' if text message, 'r' if RTTTL
 */
void tx_text(char str[], int to, char type)
{	
	int count = (strlen(str)/8)+1;			// The number of text blocks needed
	
	uint32_t data;
	uint32_t start;
	uint32_t end;
	
	if(type == 'r') 		// If sending RTTTL
	{
		data	= RTTTL;
		start	= MSTART;
		end		= MEND;
	}
	else 					// Else, must be sending TEXT
	{
		data 	= TEXT;
		start 	= TSTART;
		end 	= TEND;
	}						
	
	//-------------------------START OF TEXT BLOCK-------------------------//
	int tmp1 = (start | (count << 18));		// Add the block count
	int stmp = (tmp1 | to);					// Then a dash of target address
	send_CAN(stmp, DATA_FRAME, 0, 0);		// And serve to the CAN bus
	
	//-----------------------------TEXT BLOCK-----------------------------//
	int ttmp1 = (data | to);				// Add the target address to text block template	
	dataArray = Msg.dataA;					// txDataArray initially points the dataA array
	unsigned char i=0, j=0;					// i is the character count, j the block count
	for(i=0;i<strlen(str);i++)
	{
		if((i%8 == 0) && (i != 0))			// Don't want this to execute when i = 0, ie the first block
		{
			int ttmp = (ttmp1 | (j << 18));	// Add the block number to the text id
			
			Msg.format	= EXT_ID_FORMAT;
			Msg.id		= ttmp;
			Msg.len		= 8;	
			Msg.type	= DATA_FRAME;
			
			CAN_SendMsg(LPC_CAN2, &Msg);	// Send text block
			
			// Printing message data to screen
			int m=0;
			for(m = 0; m<4; m++)
			{
				UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, Msg.dataA[m]);
			}
			for(m = 0; m<4; m++)
			{
				UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, Msg.dataB[m]);
			}

			// Clearing message data
			Msg.dataA[0] = Msg.dataA[1] = Msg.dataA[2] = Msg.dataA[3] = 0;
			Msg.dataB[0] = Msg.dataB[1] = Msg.dataB[2] = Msg.dataB[3] = 0;
			
			j++;							// Increment the block number (ie new block)
			dataArray = Msg.dataA;			// Set array to data Array
			dataArray[i%4] = str[i];		// Add input character to the current array				
		}
		else if((i%4 == 0) && (i != 0))
		{
			dataArray = Msg.dataB;		// txDataArray now points the dataB array
			dataArray[i%4] = str[i];	// Add input character to the current array
		}
		else
		{
			dataArray[i%4] = str[i];		// Add input character to the current array
		}
	}
	
	// This is executed when the string is finished, to deal with partials
	int ttmpp = (ttmp1 | (j << 18));// Add the block number to the text id
			
	Msg.format	= EXT_ID_FORMAT;	
	Msg.id		= ttmpp;	
	Msg.len		= 8;	
	Msg.type	= DATA_FRAME;

	CAN_SendMsg(LPC_CAN2, &Msg);	// Send end of text

	int m=0;
	for(m = 0; m<4; m++)
	{
		UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, Msg.dataA[m]);
	}
	for(m = 0; m<4; m++)
	{
		UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, Msg.dataB[m]);
	}

	//-------------------------END OF TEXT BLOCK-------------------------//
	write_usb_serial_blocking("\n\r",4);
	
	Msg.format	= EXT_ID_FORMAT;	
	Msg.id		= (end | to);
	Msg.len		= 8;	
	Msg.type	= DATA_FRAME;

	CAN_SendMsg(LPC_CAN2, &Msg);	// Send end block
	
	write_usb_serial_blocking(" ",1);
	UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, j);
	write_usb_serial_blocking("\n\n\r",4);
	pre(Msg, 's');
	write_usb_serial_blocking("End of text block",17);
	write_usb_serial_blocking(" '",2);
	write_usb_serial_blocking("'",1);
	write_usb_serial_blocking("\n\r",4);
	write_usb_serial_blocking("Message id: \t",13);
	UARTPutHex32((LPC_UART_TypeDef *)LPC_UART0, Msg.id);
	write_usb_serial_blocking("\t",2);
	MSYS_Free(dataArray);
	
	
	post(Msg);
	write_usb_serial_blocking("******\n\r",10);
}

/*	
 *	end_text() is called when the end of text message block is received. 
 *	When this happens the data that has been stored in the dataArray is 
 *	dealt with. For a text message, this is printed out to the terminal 
 *	and the LCD. For an RTTTL message the data is passed to the RTTTL 
 *	handler for parsing. If morse code mode is enabled, the received text 
 *	messages are parsed to morse code.
 *	
 *	@param	msg			The received end of text message block
 */
void end_text(CAN_MSG_Type msg)
{
	int l = 0;
	write_usb_serial_blocking(" '",2);
	if(CAN_GET_TYPE(msg.id) == MMSDATA)
	{
		rtttlDecode((char*)dataArray);
		rtttl = 0;
		write_usb_serial_blocking("Received RTTTL message",22);
	}
	else if(CAN_GET_TYPE(msg.id) == VOICEDATA)
	{
		// Yet to be implemented
	}
	else if(CAN_GET_TYPE(msg.id) == SMSDATA)
	{
		for(l=0; l<(count*8); l++)
		{
			UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, dataArray[l]);
		}
		write_usb_serial_blocking("\n\r",2);
		clear_screen();
		lcdTextMsg((char*)dataArray, (count*8));
		if(morseEnable) morseParse((char*)dataArray);
	}
	write_usb_serial_blocking("'",1);
	MSYS_Free(dataArray);
}
