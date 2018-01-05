/*	
 *	@author		abradbury
 *	
 *	Morse.c receives a text string and converts it to morse code which it 
 *	plays through the DAC. A brief explanation of morse code is given here:
 *	
 *	Characters are represented using a combination of dots and dashes. The 
 *	basic unit of time is a dot and 1 dash = 3 dots. Between each dot or 
 *	dash there is a pause of 1 dot, between characters a pause of 3 dots 
 *	and a pause of 7 dots between words.
 */

#include "lpc_types.h"
#include "morse.h"
#include "serial.h"
#include "music.h"
#include "dac.h"
#include "lcd.h"
#include "string.h"
#include "ctype.h"

#define	MORSENOTE	1046.52

float 		t = 0;	// Time in milliseconds
int 		m = 0;	// Counter for the string to be converted to morse
float		morseNote = 523.26;	// The note frequency that morse code values are played at (corresponds to a 5th octave C)

/*	
 *	initMorse() initialises the morse code base typing rate. Skilled 
 *	morse code operators could type in excess of 40 WPM.
 *	
 *	@param	wordPerMin	The words per minute (WMP) to send in morse code
 */
void init_morse(int wordPerMin)
{
	write_usb_serial_blocking("Morse initialised\n\r",19);
	t = 1200/(float)wordPerMin;	
}

/*	
 *	morseParse() receives a given string and for each character in that 
 *	string, passes it to morseCode().
 *	
 *	@param	str[]		The string containing the text to be converted 
 *						to morse
 */
void morseParse(char str[])
{
	put_mult_char_lcd("Decoding message",0,1);
	put_mult_char_lcd("to Morse Code",1,2);

	for(m=0; m<=strlen(str); m++)
	{
		sineSetup();
		morseCode(str[m]);
	}
	
	clear_screen();
}

/*	
 *	dot() is the method to play a dot in morse code. It also adds the 
 *	required character pause.
 */
void dot()
{
	play(morseNote, t);
	play(0,t);
	write_usb_serial_blocking(".",1);
}

/*	
 *	dash() is the method to play the morse code dashes. It also adds the
 *	required character pause.
 */
void dash()
{
	play(morseNote, t*3);
	play(0,t);
	write_usb_serial_blocking("-",1);
}

/*	
 *	charEnd() is the method called when a character has been sent. It causes
 *	a pause of the required length.
 */
void charEnd()
{
	play(0,t*3);
	write_usb_serial_blocking(" ",1);
}

/*	
 *	wordEnd() is the method that is called when a space is detected, indicating 
 *	the end of a word. It produces the required pause.
 */
void wordEnd()
{
	play(0,t*7);
	write_usb_serial_blocking("\t",1);
}

/*	
 *	morseCode() is a method containing a huge switch statement containing 55 
 *	case statements. It matches the received character to a case, whether 
 *	this is a alpha, numeric or punctual character, and plays the correct 
 *	morse representation for this.
 *	
 *	@param	character	The character to be converted to morse
 */
void morseCode(char character)
{
	switch(tolower((int)character))
	{
		//----------Alphabet----------//
		case 'a':
			dot();
			dash();
			charEnd();
			break;
		case 'b':
			dash();
			dot();
			dot();
			dot();
			charEnd();
			break;
		case 'c':
			dash();
			dot();
			dash();
			dot();
			charEnd();
			break;
		case 'd':
			dash();
			dot();
			dot();
			charEnd();
			break;
		case 'e':
			dot();
			charEnd();
			break;
		case 'f':
			dot();
			dot();
			dash();
			dot();
			charEnd();
			break;
		case 'g':
			dash();
			dash();
			dot();
			charEnd();
			break;
		case 'h':
			dot();
			dot();
			dot();
			dot();
			charEnd();
			break;
		case 'i':
			dot();
			dot();
			charEnd();
			break;
		case 'j':
			dot();
			dash();
			dash();
			dash();
			charEnd();
			break;
		case 'k':
			dash();
			dot();
			dash();
			charEnd();
			break;
		case 'l':
			dot();
			dash();
			dot();
			dot();
			charEnd();
			break;
		case 'm':
			dash();
			dash();
			charEnd();
			break;
		case 'n':
			dash();
			dot();
			charEnd();
			break;
		case 'o':
			dash();
			dash();
			dash();
			charEnd();
			break;
		case 'p':
			dot();
			dash();
			dash();
			dot();
			charEnd();
			break;
		case 'q':
			dash();
			dash();
			dot();
			dash();
			charEnd();
			break;
		case 'r':
			dot();
			dash();
			dot();
			charEnd();
			break;
		case 's':
			dot();
			dot();
			dot();
			charEnd();
			break;
		case 't':
			dash();
			charEnd();
			break;
		case 'u':
			dot();
			dot();
			dash();
			charEnd();
			break;
		case 'v':
			dot();
			dot();
			dot();
			dash();
			charEnd();
			break;
		case 'w':
			dot();
			dash();
			dash();
			charEnd();
			break;
		case 'x':
			dash();
			dot();
			dot();
			dash();
			charEnd();
			break;
		case 'y':
			dash();
			dot();
			dash();
			dash();
			charEnd();
			break;
		case 'z':
			dash();
			dash();
			dot();
			dot();
			charEnd();
			break;
		//----------Numbers----------//
		case '0':
			dash();
			dash();
			dash();
			dash();
			dash();
			dash();
			charEnd();
			break;
		case '1':
			dot();
			dash();
			dash();
			dash();
			dash();
			charEnd();
			break;
		case '2':
			dot();
			dot();
			dash();
			dash();
			dash();
			charEnd();
			break;
		case '3':
			dot();
			dot();
			dot();
			dash();
			dash();
			charEnd();
			break;
		case '4':
			dot();
			dot();
			dot();
			dot();
			dash();
			charEnd();
			break;
		case '5':
			dot();
			dot();
			dot();
			dot();
			dot();
			charEnd();
			break;
		case '6':
			dash();
			dot();
			dot();
			dot();
			dot();
			charEnd();
			break;
		case '7':
			dash();
			dash();
			dot();
			dot();
			dot();
			charEnd();
			break;
		case '8':
			dash();
			dash();
			dash();
			dot();
			dot();
			charEnd();
			break;
		case '9':
			dash();
			dash();
			dash();
			dash();
			dot();
			charEnd();
			break;
		//----------Punctuation----------//
		case ' ':
			wordEnd();
			charEnd();
			break;
		case '.':
			dot();
			dash();
			dot();
			dash();
			dot();
			dash();
			charEnd();
			break;
		case ',':
			dash();
			dash();
			dot();
			dot();
			dash();
			dash();
			charEnd();
			break;
		case '?':
			dot();
			dot();
			dash();
			dash();
			dot();
			dot();
			charEnd();
			break;
		case '\'':
			dot();
			dash();
			dash();
			dash();
			dash();
			dot();
			charEnd();
			break;
		case '!':
			dash();
			dot();
			dash();
			dot();
			dash();
			dash();
			charEnd();
			break;
		case '/':
			dash();
			dot();
			dot();
			dash();
			dot();
			charEnd();
			break;
		case '(':
			dash();
			dot();
			dash();
			dash();
			dot();
			charEnd();
			break;
		case ')':
			dash();
			dot();
			dash();
			dash();
			dot();
			dash();
			charEnd();
			break;
		case '&':
			dot();
			dash();
			dot();
			dot();
			dot();
			charEnd();
			break;
		case ':':
			dash();
			dash();
			dash();
			dot();
			dot();
			dot();
			charEnd();
			break;
		case ';':
			dash();
			dot();
			dash();
			dot();
			dash();
			dot();
			charEnd();
			break;
		case '=':
			dash();
			dot();
			dot();
			dot();
			dash();
			charEnd();
			break;
		case '+':
			dot();
			dash();
			dot();
			dash();
			dot();
			charEnd();
			break;	
		case '-':
			dash();
			dot();
			dot();
			dot();
			dot();
			dash();
			charEnd();
			break;
		case '_':
			dot();
			dot();
			dash();
			dash();
			dot();
			dash();
			charEnd();
			break;
		case '\"':
			dot();
			dash();
			dot();
			dot();
			dash();
			dot();
			charEnd();
			break;
		case '$':
			dot();
			dot();
			dot();
			dash();
			dot();
			dot();
			dash();
			charEnd();
			break;
		case '@':
			dot();
			dash();
			dash();
			dot();
			dash();
			dot();
			charEnd();
			break;
			
		default:
			// Do nothing
			break;
	}
}
