/*	
 *	@author		abradbury
 *	
 *	Sevenseg.c is concerned with all that has to do with the 7-segment 
 *	displays. It contains methods to output digits to the displays 
 *	and clear them. 
 */

#include "serial.h"
#include "sevenseg.h"
#include "i2c.h"

#define I2C_7_SEG		0x70>>1		// Right shift to remove read/write bit
#define MAXDIGITS		2			// The max number of digits that can be placed

int array[MAXDIGITS];
int digitNum = 0;
unsigned char SEG_DATA[6] = {0x00, 0xB7, 0x40, 0x40, 0x40, 0x40};

/*	
 *	seg_rec() outputs the characters REC to 3 of the 4 7-segment 
 *	displays. Used for when recording input to the ADC, eg voice.
 */
void seg_rec()
{
	seg_clear();

	unsigned char digit1 = 0x01 | 0x20 | 0x10;					//r
	unsigned char digit2 = 0x01 | 0x20 | 0x10 | 0x40 | 0x08;	//e
	unsigned char digit3 = 0x01 | 0x20 | 0x10 | 0x08;			//c
	unsigned char digit4 = 0x00;

	unsigned char SEG_DATA[6] = {0x00, 0xB7, digit1, digit2, digit3, digit4};
	
	i2c_write(I2C_7_SEG,SEG_DATA,sizeof(SEG_DATA));
	
	write_usb_serial_blocking("7-segment: record\n\r",20);
}

/*	
 *	seg_play() outputs the characters PLAY acros the 4 7-segment 
 *	displays. It is used when playing back stored data, eg voice.
 */
void seg_play()
{
	seg_clear();

	unsigned char digit1 = 0x01 | 0x20 | 0x10 | 0x02 | 0x40;		//p
	unsigned char digit2 = 0x20 | 0x10 | 0x08;						//l
	unsigned char digit3 = 0x01 | 0x02 | 0x04 | 0x20 | 0x10 | 0x40;	//a
	unsigned char digit4 = 0x20 | 0x40 | 0x02 | 0x04 | 0x08;		//y

	unsigned char SEG_DATA[6] = {0x00, 0xB7, digit1, digit2, digit3, digit4};
	
	i2c_write(I2C_7_SEG,SEG_DATA,sizeof(SEG_DATA));
	
	write_usb_serial_blocking("7-segment: play\n\r",17);
}

/*	
 *	output() takes a digit and matches it with the relevant Hex code needed 
 *	to light the segements of the display to display the digit. It stores 
 *	this value in an array which seg_digit() write to the I2C bus.
 *	
 *	@param	number		The digit to be placed on the 7-segment display(s)
 *	@param	pair			1 if the first pair, 2 if the second.
 */
void output(int number, int pair)
{
	unsigned char value;
	
	switch(number)
	{
		case 0:
			value = 0x3F;
			break;
		case 1:
			value = 0x06;
			break;
		case 2:
			value = 0x5B;
			break;
		case 3:
			value = 0x4F;
			break;
		case 4:
			value = 0x66;
			break;
		case 5:
			value = 0x6D;
			break;
		case 6:
			value = 0x7C;
			break;
		case 7:
			value = 0x07;
			break;
		case 8:
			value = 0x7F;
			break;
		case 9:
			value = 0x67;
			break;
		default:
			value = 0x40;
			break;
	}
	if(pair == 1)	SEG_DATA[(digitNum%2)+2] = value;
	else 			SEG_DATA[(digitNum%2)+4] = value;
	digitNum++;
}

/*	
 *	seg_digit() takes a value and splits it up into single digits which it 
 *	then passes to the output() method. It then writes the completed data
 *	to the displays via the I2C bus.
 *	
 *	@param	num			The number to be placed on the 7-segment displays.
 *	@param	pair		1 if the first pair, 2 if the second.
 */
void seg_digit(int num, int pair)
{
 	int i = MAXDIGITS - 1;

 	while (num > 0) 
 	{
 		array[i--] = num % 10;
 		num = num/10;
 	}
 	
 	for(i=0;i<MAXDIGITS;i++)
 	{
 		output(array[i], pair);
 	}
 	
 	i2c_write(I2C_7_SEG,SEG_DATA,sizeof(SEG_DATA));
 }

/*	
 *	seg_clear () is a method used to clear the 7-segment displays by 
 *	writing '-' to them. '-''s are written instead of making them blank 
 *	to indicate that they are on and have been initialised/cleared.
 */
void seg_clear()
{
	unsigned char SEG_DATA[6] = {0x00, 0xB7, 0x40, 0x40, 0x40, 0x40};
	
	i2c_write(I2C_7_SEG,SEG_DATA,sizeof(SEG_DATA));
}
