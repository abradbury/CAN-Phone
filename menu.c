/*	
 *	@author		abradbury
 *	
 *	Menu.c is the main method to handle the menu system. It is a large file and hence
 *	has inline comments along with the usual comments to help with understanding. The 2
 *	main methods are menuScreen() and menuHandler() which print strings to the screen 
 *	decide upon a course of action depending on the current state, respectively. There 
 *	are also messages for text and number input and displaying text messages.
 *
 *	The recommended way to understand this code is to follow an example through it.
 *	
 *	Below is a crude map of the structure of the menu system. One shows the text printed
 *	out to the LCD, the other shows the screen numbers. The general numbering is 
 *	sequentially for scrolling lists (ie 0,1,2,3), an increase of 10 for each level 
 *	eg (10, 20, 30). However, some parts of the map do not follow this. This may be 
 *	because they reuse an existing screen (eg 10), or a screen is not needed (eg X).
 *	
 *										Welcome to the
 *						  			 	CAN Phone!													    99
 *											|											 	 		    |
 *		  -------------------------------------------------------------------			---------------------------------
 *		  |					|				|				|				|			|		|		|		|		|
 *		 Text			Ringtone		  Voice			  Other			  Inbox			0		1		2		3		4
 *		  |					|				|				|				|			|		|		|		|		|
 *	 Desk Number	   Desk Number     	Yet to be 	 Select Command:	<decoded		10		10		12		13		X
 *		  |					|		   Implemented  <list of commands>	messages>		|		|			<130-133>	|
 *	Type a message	  Choose a tone:			  			|				|			20		11				|		|
 *	Press * to send	  <list of tones>						|		   Inbox Empty		|	<110-119>			|		14
 *		  |					|								|							|		|				|
 *	   Sending...	    Sending...						Sending...						30		21				21
 *		  |					|								|							|		|				|
 *	  Message Sent	   Ringtone Sent				   Command Sent						X		31				33
 */

#include "debug_frmwrk.h"
#include "serial.h"
#include "lcd.h"
#include "keypad.h"
#include "menu.h"
#include "can.h"
#include "music.h"
#include "stdint.h"
#include "string.h"
#include "sevenseg.h"
#include "text.h"

#define WHOIS	0x14000441		// Who is? from bench 07 to broadcast
#define BOUNCE	0x1400D440		// Bounce from bench 07 to exchange
#define LOOKUP	0x14001440		// Network name lookup from bench 07 to exchange

extern int 		bufMsgs;		// The number of messages in the buffer
extern int		decMsgs;		// The number of messages that have been decoded
int				unread;			// Used for the inbox, unread = bufMsgs - decMsgs
int				morseEnable = 0;// A flag to enable morse code mode
int 			prevKey = 0;	// Used for phone-like text input
int 			currKey = 0;	// Used for phone-like text input
int 			screen = 0;		// The current screen
int 			level = 0;		// The depth level currently at
int 			z = 0;			// Used as a counter to clear the screen
int				f = 0;			// Used as a counter to display text messages on the LCD
int 			multiple = 0;	// A counter for the number of times a key is pressed in succession
int				textIntro = 0;	// A flag indicating if the text intro screen is visible
int 			bufpos = 0;		// An indexer for the LCD buffer
int 			range = 0;		// For scrolling limiter
int 			base = 0;		// For scrolling limiter
int 			menuIndex = 4;	// Used to manage the menu scrolling
int				deskDigit = 0;	// 0 if no value entered, non-zero if first digit entered
int 			destination = 0;// Holds the address that the message will be sent to
unsigned char	keyPressed = 0;	// Stores the most recent key pressed (for menuHandler())
unsigned char	keyPressed2 = 0;// Stores the most recent key pressed (for lcdTextMsg())
char 			position = '0';	// The position that text is to be placed on the LCD
char			type = '0';		// Used to indicate a text, RTTTL or voice path down the tree
char			unreadArray[4] = {'0','0','0','0'};	// An array to hold the digits of unread
char 			lcdTxtBuf[30] = {' '};	// Used to printing out text messages to the LCD
char 			lcdBuffer[100] = {' '};	// Buffers text typed on to the LCD
int 			numOfVals[10] = {3,10,4,4,4,4,4,5,4,5};	// Holds the number of values for each of the keys.
														// Eg 2 hold 4 values: a,b,c and 2.
int 			mode = 0;		/* Indicates which mode the menu system is currently in, described below:
									0:	Welcome Screen
									1:	Navigation
									2:	Text Input
									3:	Static page (no user interaction)
									4:	Action page (for example: clear screen?)
									5:	Text message display
								*/
								
/* 	The arrays below contain the values that can be outputted when a key is pressed
	when in text message mode. This simulates the values that can be outputted on a 
	traditional phone with a 0-9 keypad.
*/
char *keyType;
char one[] 		= ".,'?!\"-()1";
char two[] 		= "abc2";
char three[] 	= "def3";
char four[] 	= "ghi4";
char five[] 	= "jkl5";
char six[] 		= "mno6";
char seven[] 	= "pqrs7";
char eight[] 	= "tuv8";
char nine[] 	= "wxyz9";
char zero[] 	= " 0\n";

/*	
 *	menuScreen() is the main method to output text to the LCD representing the 
 *	different screens of the menu system. It is basically a large switch statement
 *	switching on the current screen. If there is an unrecognised screen number 
 *	the current values are printed out to the terminal and the system returns to 
 *	the main menu.
 *	
 *	@param	curScreen	The current screen which the user is at.
 *	@param	advance		1 if the user has selected that screen, else 0.
 */
void menuScreen(int curScreen, int advance)
{
	clear_screen();
	
	switch(curScreen)
	{			
		case 99:
			screen = 99;
			level = 0;
			mode = 0;
			write_usb_serial_blocking("Welcome to the CAN Phone!",25);
			put_mult_char_lcd("Welcome to the     CAN Phone!",0,0);
			break;
		case 0:
			screen = 0;
			level = 1;
			mode = 1;
			base = 0;
			range = 5;
			menuIndex = 5;
			write_usb_serial_blocking("Text",4);
			put_mult_char_lcd("Main Menu",3,1);
			put_mult_char_lcd("    Text",2,2);
			if(advance == 1)
			{
				type = 't';
				menuScreen(10,0);
			}
			break;
		case 1:
			screen = 1;
			level = 1;
			mode = 1;
			write_usb_serial_blocking("Ringtone",8);
			put_mult_char_lcd("Main Menu",3,1);
			put_mult_char_lcd("  Ringtone",2,2);
			if(advance == 1)
			{
				type = 'r';
				menuScreen(10,0);
			}
			break;
		case 2:
			screen = 2;
			level = 1;
			mode = 1;
			write_usb_serial_blocking("Voice",5);
			put_mult_char_lcd("Main Menu",3,1);
			put_mult_char_lcd("   Voice",2,2);
			if(advance == 1)
			{
				type = 'v';
				menuScreen(12,0);
			}
			break;
		case 3:
			screen = 3;
			level = 1;
			mode = 1;
			write_usb_serial_blocking("Other",5);
			put_mult_char_lcd("Main Menu",3,1);
			put_mult_char_lcd("   Other",2,2);
			if(advance == 1)
			{
				menuScreen(13,0);
			}
			break;
		case 4:
			screen = 4;
			level = 1;
			mode = 1;
			write_usb_serial_blocking("Inbox",5);
			put_mult_char_lcd("Main Menu",3,1);
			put_mult_char_lcd(" Inbox-",2,2);
			unread = bufMsgs - decMsgs;
			int urCount = 3;
			while (unread > 0) 		// Splits unread up into single digits
		 	{
		 		unreadArray[urCount--] = (char)(unread % 10)+48;
		 		unread = unread/10;
		 	}
			put_char_lcd(unreadArray[0]|0x80,9+0x40);
			put_char_lcd(unreadArray[1]|0x80,10+0x40);
			put_char_lcd(unreadArray[2]|0x80,11+0x40);
			put_char_lcd(unreadArray[3]|0x80,12+0x40);
			if(advance == 1)
			{
				inbox();
				menuScreen(14,0);
			}
			break;
			
		case 10:
			screen = 10;
			level = 2;
			mode = 2;
			write_usb_serial_blocking("Desk number:",12);
			put_mult_char_lcd("Desk number:",2,1);
			break;
		case 20:
			screen = 20;
			level = 3;
			mode = 2;
			textIntro = 1;
			put_mult_char_lcd("Type a message",0,1);
			put_mult_char_lcd("Press * to send",0,2);
			if(advance == 1)
			{
				menuScreen(30,0);
			}
			break;
		case 30:
			screen = 30;
			level = 4;
			mode = 3;
			put_mult_char_lcd("Sending...",3,1);
			break;
			
		case 11:
			screen = 11;
			level = 2;
			mode = 1;
			base = 110;
			range = 10;
			menuIndex = 10;
			put_mult_char_lcd(" Choose a tone:",2,1);
			menuScreen(110,0);
			break;
		case 110:
			screen = 110;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Abdelazer",4,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("Abdelazer:d=4,o=5,b=160:2d,2f,2a,d6,8e6,8f6,8g6,8f6,8e6,8d6,2c#6,a6,8d6,8f6,8a6,8f6,d6,2a6,g6,8c6,8e6,8g6,8e6,c6,2a6,f6,8b,8d6,8f6,8d6,b,2g6,e6,8a,8c#6,8e6,8c6,a,2f6,8e6,8f6,8e6,8d6,c#6,f6,8e6,8f6,8e6,8d6,a,d6,8c#6,8d6,8e6,8d6,2d6", destination, 'r');
				rtttlDecode("Abdelazer:d=4,o=5,b=160:2d,2f,2a,d6,8e6,8f6,8g6,8f6,8e6,8d6,2c#6,a6,8d6,8f6,8a6,8f6,d6,2a6,g6,8c6,8e6,8g6,8e6,c6,2a6,f6,8b,8d6,8f6,8d6,b,2g6,e6,8a,8c#6,8e6,8c6,a,2f6,8e6,8f6,8e6,8d6,c#6,f6,8e6,8f6,8e6,8d6,a,d6,8c#6,8d6,8e6,8d6,2d6");
				menuScreen(31,0);
			}
			break;
		case 111:
			screen = 111;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("James Bond",3,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("jamesbond:d=8,o=5,b=160:e,g,p,d#6,d6,4p,g,a#,b,2p.,g,16a,16g,f#,4p,b4,e,c#,1p", destination, 'r');
				rtttlDecode("jamesbond:d=8,o=5,b=160:e,g,p,d#6,d6,4p,g,a#,b,2p.,g,16a,16g,f#,4p,b4,e,c#,1p");
				menuScreen(31,0);
			}
			break;
		case 112:
			screen = 112;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Nokia Tune",3,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("nokiatune:d=4,o=5,b=112:8e6,8d6,f#,g#,8c#6,8b,d,e,8b,8a,c#,e,2a", destination, 'r');
				rtttlDecode("nokiatune:d=4,o=5,b=112:8e6,8d6,f#,g#,8c#6,8b,d,e,8b,8a,c#,e,2a");
				menuScreen(31,0);
			}
			break;
		case 113:
			screen = 113;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Tubular Bells",1,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("Tubular Bells:d=4,o=5,b=280:c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6", destination, 'r');
				rtttlDecode("Tubular Bells:d=4,o=5,b=280:c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6,c6,g6,c6,f6,c6,g6,c6,d#6,f6,c6,g#6,c6,a#6,c6,g6,g#6");
				menuScreen(31,0);
			}
			break;
		case 114:
			screen = 114;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Indiana Jones",1,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("IndianaJ:d=4,o=5,b=125:4e,16f,8g,2c6,4d,16e,1f,4g,16a,8b,2f6,4a,16b,4c6,4d6,4e6,4e,16f,8g,1c6,4d6,16e6,2f6,4g,16g,4e6,4d6,16g,4e6,4d6,16g,4f6,4e6,16d6,2c6", destination, 'r');
				rtttlDecode("IndianaJ:d=4,o=5,b=125:4e,16f,8g,2c6,4d,16e,1f,4g,16a,8b,2f6,4a,16b,4c6,4d6,4e6,4e,16f,8g,1c6,4d6,16e6,2f6,4g,16g,4e6,4d6,16g,4e6,4d6,16g,4f6,4e6,16d6,2c6");
				menuScreen(31,0);
			}
			break;
		case 115:
			screen = 115;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Thunderbirds",2,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("Thunderb:d=4,o=5,b=125:8g#,16f,16g#,4a#,8p,16d#,16f,8g#,8a#,8d#6,16f6,16c6,8d#6,8f6,2a#,8g#,16f,16g#,4a#,8p,16d#,16f,8g#,8a#,8d#6,16f6,16c6,8d#6,8f6,2g6,8g6,16a6,16e6,4g6,8p,16e6,16d6,8c6,8b,8a,16b,8c6,8e6,2d6", destination, 'r');
				rtttlDecode("Thunderb:d=4,o=5,b=125:8g#,16f,16g#,4a#,8p,16d#,16f,8g#,8a#,8d#6,16f6,16c6,8d#6,8f6,2a#,8g#,16f,16g#,4a#,8p,16d#,16f,8g#,8a#,8d#6,16f6,16c6,8d#6,8f6,2g6,8g6,16a6,16e6,4g6,8p,16e6,16d6,8c6,8b,8a,16b,8c6,8e6,2d6");
				menuScreen(31,0);
			}
			break;
		case 116:
			screen = 116;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Inspect Gadget",1,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("Insepect:d=4,o=5,b=200:8g,8a,8p,8f,8p,8g#,8p,8e,8p,8g,8p,8f,8p,8d,8e,8f,8g,8a,8p,4d6,2c#6,2p,8d,8e,8f,8g,8a,8p,8f,8p,8g#,8p,8e,8p,8g,8p,8f,8p,4d,2p,4c#,4d", destination, 'r');
				rtttlDecode("Insepect:d=4,o=5,b=200:8g,8a,8p,8f,8p,8g#,8p,8e,8p,8g,8p,8f,8p,8d,8e,8f,8g,8a,8p,4d6,2c#6,2p,8d,8e,8f,8g,8a,8p,8f,8p,8g#,8p,8e,8p,8g,8p,8f,8p,4d,2p,4c#,4d");
				menuScreen(31,0);
			}
			break;
		case 117:
			screen = 117;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Superman",4,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("SuperMan:d=4,o=5,b=180:8g,8g,8g,c6,8c6,2g6,8p,8g6,8a.6,16g6,8f6,1g6,8p,8g,8g,8g,c6,8c6,2g6,8p,8g6,8a.6,16g6,8f6,8a6,2g.6,p,8c6,8c6,8c6,2b.6,g.6,8c6,8c6,8c6,2b.6,g.6,8c6,8c6,8c6,8b6,8a6,8b6,2c7,8c6,8c6,8c6,8c6,8c6,2c.6", destination, 'r');
				rtttlDecode("SuperMan:d=4,o=5,b=180:8g,8g,8g,c6,8c6,2g6,8p,8g6,8a.6,16g6,8f6,1g6,8p,8g,8g,8g,c6,8c6,2g6,8p,8g6,8a.6,16g6,8f6,8a6,2g.6,p,8c6,8c6,8c6,2b.6,g.6,8c6,8c6,8c6,2b.6,g.6,8c6,8c6,8c6,8b6,8a6,8b6,2c7,8c6,8c6,8c6,8c6,8c6,2c.6");
				menuScreen(31,0);
			}
			break;
		case 118:
			screen = 118;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Star Trek",3,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("Star Trek:d=4,o=5,b=063:8f.,16a#,d#.6,8d6,16a#.,16g.,16c.6,f6",destination,'r');
				rtttlDecode("Star Trek:d=4,o=5,b=063: 8f.,16a#,d#.6,8d6,16a#.,16g.,16c.6,f6");
				menuScreen(31,0);
			}
			break;
		case 119:
			screen = 119;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose a tone:",1,1);
			put_mult_char_lcd("Star Wars",3,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				tx_text("StWars:d=4,o=5,b=180:8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6,p,8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6",destination, 'r');
				rtttlDecode("StWars:d=4,o=5,b=180:8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6,p,8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6");
				menuScreen(31,0);
			}
			break;
		case 21:
			screen = 21;
			level = 3;
			mode = 3;
			put_mult_char_lcd("Sending...",3,0);
			break;
		case 31:
			screen = 31;
			level = 4;
			mode = 3;
			clear_screen();
			put_mult_char_lcd("Ringtone Sent",1,1);
			delay(7000);
			menuScreen(0,0);
			break;
						
		case 12:
			screen = 12;
			level = 2;
			mode = 3;
			put_mult_char_lcd("Yet to be ",3,1);
			put_mult_char_lcd("Implemented",2,2);
			delay(7000);
			menuScreen(0,0);
			break;
			
		case 13:
			screen = 13;
			level = 2;
			mode = 1;
			base = 130;
			range = 4;
			menuIndex = 4;
			put_mult_char_lcd("Choose command:",0,1);
			menuScreen(130,0);
			break;
		
		case 130:
			screen = 130;
			level = 2;
			mode = 1;	
			put_mult_char_lcd("Choose command:",0,1);
			put_mult_char_lcd("Who Is Online?",1,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				send_CAN(WHOIS, DATA_FRAME, 0x00, 0x00);
				menuScreen(33,0);
			}
			break;
		case 131:
			screen = 131;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose command:",0,1);
			put_mult_char_lcd("Number Lookup",2,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				send_CAN(LOOKUP, DATA_FRAME, 0x00, 0x00);
				menuScreen(33,0);
			}
			break;
		case 132:
			screen = 132;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose command:",0,1);
			put_mult_char_lcd("Bounce",4,2);
			if(advance == 1)
			{
				menuScreen(21,0);
				send_CAN(BOUNCE, DATA_FRAME, 0x00, 0x00);
				menuScreen(33,0);
			}
			break;
		case 133:
			screen = 133;
			level = 2;
			mode = 1;
			put_mult_char_lcd("Choose command:",0,1);
			put_mult_char_lcd("Morse Mode",3,2);
			if(advance == 1)
			{
				if(morseEnable == 1)
				{
					morseEnable = 0;
					clear_screen();
					put_mult_char_lcd("Morse Off",4,2);
				}
				else if(morseEnable == 0)
				{
					morseEnable = 1;
					clear_screen();
					put_mult_char_lcd("Morse On",5,2);
				}
				delay(7000);
				menuScreen(0,0);	
			}
			break;
		case 33:
			screen = 33;
			level = 4;
			mode = 3;
			clear_screen();
			put_mult_char_lcd("Command Sent",2,1);
			delay(4000);
			menuScreen(0,0);
			break;
			
		case 14:
			screen = 14;
			level = 2;
			mode = 3;
			put_mult_char_lcd("Inbox Empty",2,1);
			delay(7000);
			menuScreen(0,0);
			break;
			
		default:
			put_mult_char_lcd("Error",6,0);
			
			write_usb_serial_blocking("Error - screen(",15);
			UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, screen);
			write_usb_serial_blocking("), level(",9);
			UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, level);
			write_usb_serial_blocking("), mode(",8);
			UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, mode);
			write_usb_serial_blocking("), menuIndex(",13);
			UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, menuIndex);
			write_usb_serial_blocking("), base(",8);
			UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, base);
			write_usb_serial_blocking("), range(",9);
			UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, range);
			write_usb_serial_blocking(").\n\r",4);
			
			mode = 1;
			delay(7000);
			menuScreen(0,0);
			break;
	}
	
	if(mode == 1)
	{
		put_char_lcd(0x10, 0x40);		// Left arrow
		put_char_lcd(0x20, 0x40+15);	// Right arrow
	}
}

/*	
 *	menuHandler() computes what actions are to be taken when a key is pressed based 
 *	on what the current variables are. It is a large case statement that switches on 
 *	the key pressed. There are lots of inline comments to aid understanding. At the 
 *	end of the method is a recursive call.
 *	
 *	@param	k			The key pressed 
 */
void menuHandler(unsigned char k)
{
	switch(k)
	{
		case 0xB1 :					// 1
			if(mode == 0)			// Welcome
			{
				write_usb_serial_blocking("1",1);
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 1;
					keyType = one;	// .,'?|"=()1
					textEntry(keyType,1);
					prevKey = 1;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('1');
				}
			}
			break;
		case 0xB2 :					// 2
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 2;	
					keyType = two;	// abc2
					textEntry(keyType,2);
					prevKey = 2;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('2');
				}
			}
			break;
		case 0xB3 :					//3
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 3;
					keyType = three;// def3
					textEntry(keyType,3);
					prevKey = 3;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('3');
				}
			} 
			break;
		case 0xC1 :					//A
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 11;
					textEntry(" ",11);// Advance, for double letters
					prevKey = 11;
				}
			} 
			else if(mode == 4)
			{
				put_mult_char_lcd("  Clearing....", 0, 0);
				delay(10000);
				for(z=0; z<=bufpos; z++) lcdBuffer[z] = '\0';
				clear_screen();
				position = 0;
				bufpos = 0;
			}
			break;
		case 0xB4 :					//4
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 1)		// Left key
			{
				menuIndex--;
				menuScreen(limit(menuIndex),0);
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 4;
					keyType = four;	// ghi4
					textEntry(keyType,4);
					prevKey = 4;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('4');
				}
			} 
			break;
		case 0xB5 :					//5
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 1)
			{
				menuScreen(limit(menuIndex),1);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 5;
					keyType = five;	// jkl5
					textEntry(keyType,5);
					prevKey = 5;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('5');
				}
			}
			break;
		case 0xB6 :					//6
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 1)		// Right key
			{
				menuIndex++;
				menuScreen(limit(menuIndex),0);
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 6;
					keyType = six;	// mno6
					textEntry(keyType,6);
					prevKey = 6;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('6');
				}
			}
			break;
		case 0xC2 :					//B
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 4)
			{
				clear_screen();
				put_mult_char_lcd(lcdBuffer,0,0);//restore	
			}
			
			break;
		case 0xB7 :					//7
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 7;
					keyType = seven;// pqrs7
					textEntry(keyType,7);
					prevKey = 7;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('7');
				}
			}
			break;
		case 0xB8 :					//8
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 8;
					keyType = eight;// tuv8
					textEntry(keyType,8);
					prevKey = 8;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('8');
				}
			}
			break;
		case 0xB9 :					//9
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					currKey = 9;
					keyType = nine;	// wxyz9
					textEntry(keyType,9);
					prevKey = 9;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('9');
				}
			}
			break;
		case 0xC3 :					//C
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				// Clear screen
				currKey = 13;
				textEntry(" ",13);
				prevKey = 13;
			}
			break;
		case 0xAA :					//*
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);	// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)	// Text msg input
				{
					menuScreen(30,0);
					tx_text(lcdBuffer, destination, 't');
					for(z=0; z<=bufpos; z++) lcdBuffer[z] = ' ';
					clear_screen();
					put_mult_char_lcd("Message Sent",3,1);
					delay(2000);
					menuScreen(0,0);
				}
				else menuScreen(0,0);
			}
			else
			{
				menuScreen(0,0);
			}
			break;
		case 0xB0 :					//0
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);		// Text
				delay(800);
			}
			else if(mode == 2)
			{
				if(screen == 20)		// Text msg input
				{
					currKey = 0;
					keyType = zero;	// 0 \n
					textEntry(keyType,0);
					prevKey = 0;
				}
				else if(screen == 10)// Desk number input
				{
					numberEntry('0');
				}
			}
			break;
		case 0xA3 :					//#
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);		// Text
				delay(800);
			}
			break;
		case 0xC4 :					//D
			if(mode == 0)			// Welcome
			{
				menuScreen(0,0);		// Text
				delay(800);
			}
			else if(mode == 2)
			{
				currKey = 14;
				textEntry(" ",14);
				prevKey = 14;
			}
			break;
		default:						//Any other key
			// Do nothing
			break;
	} 
	
	keyPressed = key_to_charcode(readkey());
	
	do
	{
		menuHandler(keyPressed);
	}
	while(1);
	
	clear_screen();
	put_mult_char_lcd("End",0,0);
	
}

/*	
 *	limit() restricts the screen to a set range. For example when at screen
 *	4 and move right, menuIndex is incremented to 4 and this is passed to 
 *	limit() which returns screen as 0, so it loops back to the text screen.
 *	
 *	@param	input		The key pressed 
 */
int limit(int input)
{	
	screen = base + input%range;
	return screen;
}

/*	
 *	textEntry() deals with when the system is in text entry mode so that the
 *	keypad acts like a phone keypad. For example, pressing key 2 twice would 
 *	output a 'b'. The C key is used to clear the screen, D for deleting a 
 *	single character, * is used to send.
 *
 *	It was planned to have it so that if the same key is pressed within X ms 
 *	of the previous key press then the value would increment (eg a -> b). 
 *	A simpler solution was to just increment the character if the previous 
 *	key pressed was the same as the current key pressed. However, this introduced 
 *	a problem; if you wanted to type a double letter as in 'hello', you couldn't.
 *	To rectify this, the A key was assigned to 'advance'. So when A is pressed 
 *	the next characte is place on the following position to the current character.
 *	
 *	@param	inputValues[]	A character array contain the characters that
 *							can be output when the key is pressed in text
 *							entry mode. 
 *	@param	key				The key that was pressed
 */
void textEntry(char inputValues[], int key)
{
	if(textIntro == 1)
	{
		clear_screen();
		position = 0;
		textIntro = 0;
		bufpos = 0;
	}
	
	if(key == 14)		// D key for delete
	{
		put_char_lcd(0xA0,position);
		lcdBuffer[bufpos] = '\0';
		write_usb_serial_blocking("\n\rDELETE",10);
		delay(800);
		position--;
		bufpos--;
	}
	else if(key == 11)	// A key for advance, for double letters
	{
		position++;
		write_usb_serial_blocking("\n\rADVANCE",11);
		delay(800);
		position--;
	}
	else if(key == 13)	// C key for clear screen
	{
		clear_screen();
		put_mult_char_lcd(" Clear screen?", 0, 0);
		mode = 4;
		menuHandler(key_to_charcode(readkey()));
		mode = 2;
		write_usb_serial_blocking("\n\rCLEARED",11);
		delay(800);
	}
	else if(prevKey == currKey)	// Multiple presses of the same key
	{
		multiple++;
		put_char_lcd(keyType[multiple%numOfVals[key]]|0x80,position);
		UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, keyType[multiple%numOfVals[key]]);
		
		lcdBuffer[bufpos] = keyType[multiple%numOfVals[key]];
		write_usb_serial_blocking("{",1);
		for(z=0; z<bufpos+1; z++) UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, lcdBuffer[z]);
		write_usb_serial_blocking("}",1);
		UARTPutDec16((LPC_UART_TypeDef *)LPC_UART0, position);
		write_usb_serial_blocking("/",1);
		UARTPutDec16((LPC_UART_TypeDef *)LPC_UART0, bufpos);
		
		delay(800);
	}
	else							// A standard key press
	{	
		position++;
		bufpos++;
		if(position==16) position=0x40;
		if(position>(0x40+15)) position=0;
		put_char_lcd(keyType[0]|0x80,position);
		write_usb_serial_blocking("\n\r",4);
		UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, keyType[0]);

		lcdBuffer[bufpos] = keyType[0];
		write_usb_serial_blocking("{",1);
		for(z=0; z<=bufpos; z++) UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, lcdBuffer[z]);
		write_usb_serial_blocking("}",1);
		UARTPutDec16((LPC_UART_TypeDef *)LPC_UART0, position);
		write_usb_serial_blocking("/",1);
		UARTPutDec16((LPC_UART_TypeDef *)LPC_UART0, bufpos);
		
		delay(800);
	}
}

/*	
 *	numberEntry() is the method called when only numbers are needed to be 
 *	entered via the keypad, for example when inputting the desk number.
 *	
 *	It checks if the deskDigit variable is 0, if so then it assumes the 
 *	value passed is the first digit and sets deskDigit to this. When the 
 *	second digit is passed in deskDigit will not be 0, so it treats it as 
 *	the second digit. Then the inputted value is checked to see if it is
 *	in range. If so, then a screen is returned to depending on which 
 *	thread of the menu system the user is in. Else, the user is asked to 
 *	try again.
 *
 *	Note that this method is unable to accept 0 (broadcast address) or 
 *	any single digit. This would have been rectified if there had been 
 *	more time.
 *	
 *	@param	value		The value of the key pressed 
 */
void numberEntry(char value)
{
	write_usb_serial_blocking("Num Entry",9);
	if(deskDigit == 0)
	{
		deskDigit = (int)value-48;
		position = 7+0x40;
		
		put_char_lcd(value|0x80,position);
		UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, value);
	}
	else
	{
		destination = (10*deskDigit) + (int)(value-48);
		position = 8+0x40;
		deskDigit = 0;
		
		put_char_lcd(value|0x80,position);
		UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, value);
		
		position = 0;
		
		delay(4000);
		
		if(destination > 45 || (destination > 2 && destination < 11))
		{
			clear_screen();
			put_mult_char_lcd("Range Error",3,1);
			put_mult_char_lcd("Please try again",0,2);
			deskDigit = 0;
			destination = 0;
			delay(10000);
			menuScreen(10,0);
		}
		else if(type == 't')		// If text thread
		{
			delay(5000);
			menuScreen(20,0);
		}
		else if(type == 'r')		// If RTTTL thread
		{
			delay(5000);
			menuScreen(11,0);
		}
		else if(type == 'v')		// If voice thread
		{
			// Not yet implemented
		}
	}
	
	write_usb_serial_blocking(" - deskDigit(",13);
	UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, deskDigit);
	write_usb_serial_blocking("), value(",9);
	UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, value);
	write_usb_serial_blocking("), position(",12);
	UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, position);
	write_usb_serial_blocking(").\n\r",4);
	
	delay(800);
}

/*	
 *	lcdTextMsg() is a method for displaying received messages on the LCD.
 *	It places 30 characters at a time on the LCD with the ability to scroll 
 *	up and down to read the text message. It does this until 'B' is pressed.
 *	To aid the user experience, when scrolling down, for example, line 1 is 
 *	no longer displayed, though line 2 is, now occupying line 1. This is to
 *	remind the user what they just read.
 *	
 *	@param	text			The received text to display 
 *	@param	size			The number of characters 
 */
void lcdTextMsg(char text[], int size)
{
	mode = 5;
	int counter = 0;
	
	do
	{
		clear_screen();
		
		put_char_lcd(0x12, 0);		// up arrow
		put_char_lcd(0x30, 0x40);	// down arrow
		
		for(f = 0; f < 30; f++)
		{
			lcdTxtBuf[f] = text[f+counter];
		}
	
		put_mult_char_lcd(lcdTxtBuf, 1, 0);
	
		keyPressed2 = key_to_charcode(readkey());
		delay(800);
	
		if(keyPressed2 == 0xB2)			// Scroll up
		{
			if(counter >= 15)	counter = counter - 15;
		}
		else if (keyPressed2 == 0xB8)	// Scroll down
		{
			if(counter < (size - 15)) 	counter = counter + 15;
		}
	}
	while(!(keyPressed2 == 0xC2));
	
	clear_screen();
	
	mode = 2;
	delay(800);
}

/*	
 *	inbox() is a rudimentary inbox method which calls the receiveBufferHandler()
 *	method. This deciphers all the buffered messages until the buffer is empty.
 */
void inbox()
{
	while(bufMsgs != 0)
	{
		receiveBufferHandler();
	}
	seg_clear();
}
