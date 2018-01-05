/*	
 *	Serial.c contains the main entry point for the program, the serial line setup 
 *	method, methods to allow reading and writing to the USB cable and a basic delay 
 *	method. It is based on example code provided by the university.
 */
 
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"
#include "lpc_types.h"
#include "serial.h"
#include "stdio.h"
#include "can.h"
#include "i2c.h"
#include "debug_frmwrk.h"
#include "keypad.h"
#include "string.h"
#include "lcd.h"
#include "text.h"
#include "dac.h"
#include "sevenseg.h"
#include "music.h"
#include "menu.h"
#include "morse.h"

/*	
 *	main() is the main entry point into the program, it is from 
 *	here that all other methods are called.
 */
void main(void)
{
	serial_init();
	
	write_usb_serial_blocking("\n\r**************\n\r",20);
	write_usb_serial_blocking("Program started \n\r",20);
	
	init_i2c();
	init_lcd();
	init_DAC();
	seg_clear();
	init_CAN();
	init_morse(25);
	write_usb_serial_blocking("\n\r",2);
	menuScreen(99,0);
	
	write_usb_serial_blocking("\n\r",2);
	menuHandler(readkey());

	write_usb_serial_blocking("Finished main\n\r",16);
}

/*	
 *	delay() is a basic delay function.
 *	
 *	@param	tick			A value to delay by.
 */
void delay (unsigned int tick)
{
	volatile int i = 0;
	while (i < (tick*1024))
	{
		i++;
	}
}

/*	
 *	read_usb_serial_blocking() reads text from the USB line. This can  
 *	be read via a terminal screen on a computer.
 *	
 *	@param	buf			A pointer to the receive buffer.
 *	@param	length		The length of the text string to be received.
 *	@return				The number of bytes received.
 */
int read_usb_serial_none_blocking(char *buf,int length)
{
	return(UART_Receive((LPC_UART_TypeDef *)LPC_UART0, (uint8_t *)buf, length, NONE_BLOCKING));
}

/*	
 *	write_usb_serial_blocking() writes text to the USB line. This can  
 *	be viewed on a terminal screen on a computer.
 *	
 *	@param	buf			The text to be sent.
 *	@param	length		The length of the text string to be sent.
 *	@return				The number of bytes sent.
 */
int write_usb_serial_blocking(char *buf,int length)
{
	return(UART_Send((LPC_UART_TypeDef *)LPC_UART0,(uint8_t *)buf,length, BLOCKING));
}

/*	
 *	serial_init() initialises the USB serial line by setting the send 
 *	and receive pins and the structs which are descibed below.
 */
void serial_init(void)
{
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum 		= 1;
	PinCfg.OpenDrain 	= 0;
	PinCfg.Pinmode 		= 0;
	PinCfg.Portnum		= 0;
	PinCfg.Pinnum 		= 2;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum 		= 3;
	PINSEL_ConfigPin(&PinCfg);
	
	// Initialises UARTConfigStruct with its default value of 9600 bps, 
	// 8-bit data, 1 stopbit and no parity.
	UART_CFG_Type UARTConfigStruct;
	UART_ConfigStructInit(&UARTConfigStruct);
	UART_Init((LPC_UART_TypeDef *)LPC_UART0, &UARTConfigStruct);	
	
	// Initialises UARTFIFOConfigStruct with its default values; DMA disabled,
	// reset receive and send buffers and receive trigger level as 0.
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;	
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART0, &UARTFIFOConfigStruct);

	// Enables the transmission on the UART send pin.
	UART_TxCmd((LPC_UART_TypeDef *)LPC_UART0, ENABLE);
}
