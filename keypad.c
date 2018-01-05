/*	
 *	@author		abradbury
 *	
 *	Keypad.c deals with reading the keypad and detecting which key is pressed.
 */

#include "i2c.h"
#include "lcd.h"
#include "serial.h"
#include "keypad.h"

#define KEYPAD		0x21

unsigned char 	buf[1];		// Used in detecting which key is pressed

/*	
 *	readkey() detects key presses on the keypad using the row-column matrix. 
 *	
 *	@return	buf			A value representing which key is pressed
 */
unsigned char readkey()
{
	unsigned char buf[1];
	
	while(1)
	{
		buf[0]=0xF7;
		i2c_write(KEYPAD,buf,1);
		i2c_read(KEYPAD,buf,1);
		if(buf[0]!=0xF7){break;};

		buf[0]=0xFb;
		i2c_write(KEYPAD,buf,1);
		i2c_read(KEYPAD,buf,1);
		if(buf[0]!=0xFb){break;};

		buf[0]=0xFd;
		i2c_write(KEYPAD,buf,1);
		i2c_read(KEYPAD,buf,1);
		if(buf[0]!=0xFd){break;};

		buf[0]=0xFe;
		i2c_write(KEYPAD,buf,1);
		i2c_read(KEYPAD,buf,1);
		if(buf[0]!=0xFe){break;};
	};
	return buf[0];
}

/*	
 *	key_to_charcode takes in an input from readkey() and matches it to the 
 *	corresponding key value.
 *	
 *	@param	c			A hex value representing a key press on the matrix
 *	@return	c			The hex code for the key's label
 */
unsigned char key_to_charcode(unsigned char c)
{

	switch(c)
	{
		case 0x77 :
			  return 0xB1;	//1
		case 0xB7 :
			  return 0xB2;	//2
		case 0xD7 :
			  return 0xB3; 	//3
		case 0xE7 :
			  return 0xC1;	//A

		case 0x7B :
			  return 0xB4; 	//4
		case 0xBB :
			  return 0xB5; 	//5
		case 0xDB :
			  return 0xB6; 	//6
		case 0xEB :
			  return 0xC2;	//B

		case 0x7D :
			  return 0xB7; 	//7
		case 0xBD :
			  return 0xB8; 	//8
		case 0xDD :
			  return 0xB9; 	//9
		case 0xED :
			  return 0xC3; 	//C

		case 0x7E :
			  return 0xAA; 	//*
		case 0xBE :
			  return 0xB0; 	//0
		case 0xDE :
			  return 0xA3; 	//#
		case 0xEE :
			  return 0xC4; 	//D
	}

	return c;
}
