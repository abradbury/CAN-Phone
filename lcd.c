/*	
 *	@author		abradbury
 *	
 *	Lcd.c deals with the LCD on the device. It initialises it based on various
 *	parameters, includes methods to print characters and strings to the LCD and 
 *	to clear the screen.
 */
 
#include "serial.h"
#include "lcd.h"
#include "i2c.h"
#include "string.h"

#define I2C_BATRON_LCD		0x3B

char posi = 0;				// Holds the current position when writing to the LCD
char lcdBuffer[100];		// Buffer for LCD output, limited to 100 characters

/*	
 *	init_lcd() is used to set the lcd screen using various parameters. These are
 *	all set by changing values which are place in an array and written to the 
 *	LCD over the I2C bus. A consequence of the initialisation is that the screen 
 *	is populated with arrows. To rectify this, the screen is cleared after 
 *	initialisation. 
 *	
 *	A detailed breakdown of the customisable options is given inline. B1 refers to
 *	bit 4
 */
void init_lcd()
{
	unsigned char LCD_INIT_DATA[12] = {
			// 7654 3210
	0x00,	// 0000 0000	NOP or RS = 0
		
	0x34,	// 0011 0100	Function set	B4 data length, B2 no. of display lines, B1 single line/MUX 1:9, B0 instr set
				// 8 bits, 2x16 display, MUX 1:18 (2x16 character display), basic instr set

	0x0f,	// 0000 1011	Display control	B2 display, B1 cursor, B0 blink 
				// display on, cursor on, blink on

	0x06,	// 0000 0110	Cursor move dir	B1 Inc/Dec, B0 shift
				// increment, display freeze

	0x35,	// 0011 0101	Function set	B4 data length, B2 no. of display lines, B1 single line/MUX 1:9, B0 instr set	
				// 8 bits, 2x16 display, MUX 1:18 (2x16 character display), ext instr set

	0x04,	// 0000 0100	?Display config	B1 column data, B0 row data
				// Column data left to right, row data top to bottom

	0x10,	// 0001 0000	?Temp control	B1 TC1, B0 TC0 - used to set the temp coefficient	
				// Temp coefficient 0: typical -0.16%/K

	0x42,	// 0100 0010	?Set HVgen	B1 S1, B0 S0
				// internal HVgen stages to 3 (4x voltages multiplier)

	0x9f,	// 1001 1111	?Set LCD voltage	B6 V, B5-0 voltage
				// sets VA to 011111 (31d, 0x1F)

	0x34,	// 0011 0100	Function set	DB4 data length, DB2 no. of display lines, DB1 single line/MUX 1:9, DB0 instr set
				// 8 bits, 2x16 display, MUX 1:18 (2x16 character display), basic instr set

	0x80,	// 1000 0000	Set DDRAM addr
				// Set DDRAM addr to 0x00	

	0x02	// 0000 c return types0010	Return home	
	};

	i2c_write(I2C_BATRON_LCD,LCD_INIT_DATA,sizeof(LCD_INIT_DATA));
	
	clear_screen();

	write_usb_serial_blocking("LCD initialised \n\r",19);
}

/*	
 *	put_char_lcd() is used to place single chacaters at a specific position on the  
 *	LCD. The input paramaters are added to an array which is sent to the LCD. The 
 *	first element (0x80) is the command code to set the DDRAM, the second (pos |
 *	0x80) is the position in bank1 of the DDRAM, the third (0x40) is the command
 *	code for writing to DDRAM and the final element is the character with bit 7 
 *	set to select character set R. 
 *	
 *	@param	chr			The character to be written to the LCD. ASCII characters 
 *						need to be 'or-ed' with 0x80 to display correctly.
 *	@param	pos			The position that the character is to be written to.
 *						0-15 for the first line, 64 to 79 for the second.
 */
void put_char_lcd(unsigned char chr, unsigned char pos)
{
	unsigned char data[4] = {0x80, pos | 0x80, 0x40, chr};
	
	i2c_write(I2C_BATRON_LCD, data, 4);
}

/*	
 *	put_mult_char_lcd() is a method to place a string of characters to the LCD. 
 *	
 *	@param	input		The string of characters to be written to the LCD
 *	@param	offset		The number of positions the text is to be offset from the 
 *						left of the screen.
 *	@param	line			0 if the text is to overflow from the 1st to 2nd line and 
 *						loop back, 1 if the text is to be limited to the 1st line
 *						only and 2 if the text is to be limited to the 2nd line.
 */
void put_mult_char_lcd(char input[], char offset, int line)
{
	unsigned char i;
	if(line == 0)
	{
		posi = offset;
		for(i=0;i<strlen(input);i++)
		{
			put_char_lcd(input[i]|0x80, posi);
			posi++;
			if(posi==16)		posi=0x40+offset;
			if(posi>(0x40+15))	posi=0;
		
		}
	}
	else if(line == 1)
	{
		posi = offset;
		for(i=0;i<strlen(input);i++)
		{
			put_char_lcd(input[i]|0x80, posi);
			posi++;
			if(posi>15)	posi=0;		
		}
	}
	else if(line == 2)
	{
		posi = offset+0x40;
		for(i=0;i<strlen(input);i++)
		{
			put_char_lcd(input[i]|0x80, posi);
			posi++;
			if(posi<16)			posi=0x40;
			if(posi>(0x40+15))	posi=0;		
		}
	}
}

/*	
 *	clear_screen() is a simple method to clear the screen. It does this by 
 *	writing 0xA0, a blank, to every position on the LCD.
 */
void clear_screen()
{
	unsigned char i;
	for(i=0;i<16;i++)
		{
			put_char_lcd(0xA0,i);
			put_char_lcd(0xA0,0x40+i);
		}	
}
