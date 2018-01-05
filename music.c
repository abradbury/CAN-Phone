/*	
 *	@author		abradbury
 * 
 *	Music.c handles received RTTTL messages. It parses the received data, and 
 *	creates the frequency and duration values to be played, finally it plays 
 *	the received message.
 */

#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"
#include "debug_frmwrk.h"
#include "serial.h"
#include "music.h"
#include "dac.h"
#include "lcd.h"
#include "string.h"
#include "ctype.h"

char			name[32];		// Array to hold RTTTL name (spec limit is 10 characters)
char			defaults[32];	// Array to hold the default duration, ocatve and bpm
char			data[255];		// Array to hold the RTTTL data
float 			freq[255];		// Array to hold the calculated RTTTL frequencies
float 			dura[255];		// Array to hold the calculated RTTTL durations

int 			ddur = 0;		// Default duration
int 			doct = 0;		// Default ocatve
int 			dbpm = 0;		// Default BPM
int 			k = 0;			// Data size counter
int 			x = 0;			// Frequency and duration size counter
float 			prevNote = 0;	// Holds the previous note played, to introduce a gap

TIM_TIMERCFG_Type	Timer0;		// The timer struct used for note timing
TIM_MATCHCFG_Type	Match0;		// The match struct used for note timing

/*	
 *	rtttlDecode() is a common entry method for decoding received RTTTL messages. 
 *	It calls the 3 main methods that are part of the decode process. It also 
 *	outputs the name of the received song to the LCD. When the song has been
 *	played, it resets some variables and sets the arrays to 0. 
 *	
 *	@param	str[]		The string received
 */
void rtttlDecode(char str[])
{
	sineSetup();
	rtttlSplit(str);
	
	clear_screen();
	put_mult_char_lcd("Playing", 1, 1);
	put_mult_char_lcd(name, 1, 2);
	
	rtttlDefaults(str);
	rtttlData(str);
	clear_screen();
	
	ddur = 0;
	doct = 0;
	dbpm = 0;
	k = 0;
	x = 0;
	
	memset(name,0, sizeof(name));
	memset(defaults,0, sizeof(defaults));
	memset(data,0, sizeof(data));
	memset(freq,0, sizeof(freq));
	memset(dura,0, sizeof(dura));
}

/*	
 *	rtttlSplit() scans through the input string until the colon delimiters 
 *	are found. 
 *	 - The data before the 1st colon is stored in the name array. The RTTTL 
 *	   specification sets a maximum name length of 10 characters, but the 
 *	   implementation below can handle up to 32.
 *	 - The data between the 1st and 2nd colons contains the default duration, 
 *	   ocatve and bpm (beats per minute) values.
 *	 - The data after the 2nd colon is the main musical data. This is stored 
 *     in the data array. A comma is added to the end of this data to allow 
 *	   the rtttlData() method to detect the final value.
 *	
 *	@param	str[]		The string received
 */
void rtttlSplit(char str[])
{
	int i=0,j=0;		// i input string and name index, j defaults index
	
	while(str[i] != ':')	
	{
		name[i] = str[i];
		i++;
	}

	write_usb_serial_blocking(" Name:     ",11);
	UARTPuts((LPC_UART_TypeDef *)LPC_UART0, name);
	i++;

	while(str[i] != ':')	
	{	
		defaults[j] = str[i];
		j++;
		i++;
	}

	defaults[j] = ',';
	j++;

	write_usb_serial_blocking("\n\r Defaults: ",15);
	UARTPuts((LPC_UART_TypeDef *)LPC_UART0, defaults);
	i++;

	
	while(str[i] != '\0')
	{
		data[k] = str[i];
		k++;					// k has been made global for access by rtttlData()
		i++;
	}
	
	data[k] = ',';
	k++;

	write_usb_serial_blocking("\n\r Data:     ",15);
	UARTPuts((LPC_UART_TypeDef *)LPC_UART0, data);
}

/*	
 *	rtttlDefaults() parses the string containing the default values for the 
 *	RTTTL song being parsed. It is a specialised method for the RTTTL 
 *	defaults specification. It iterates through the string using the commas 
 *	as delimiters. This implementation can deal with spaces between the values.
 *	
 *	@param	str[]		The string containing the default values
 */
void rtttlDefaults(char str[])
{
	int p = 0;
	
	while(defaults[p] != ',')
	{
		if(digit(defaults[p]) && ddur == 0)
		{
			ddur = (int)defaults[p]-48;
		}
		else if(digit(defaults[p]) && ddur != 0)
		{
			ddur = (ddur*10) + (int)defaults[p]-48;
		}
		p++;
	}
	p++;
	
	while(defaults[p] != ',')
	{
		if(digit(defaults[p]) && doct == 0)
		{
			doct = (int)defaults[p]-48;
		}
		p++;
	}
	p++;
	
	while(defaults[p] != ',')
	{		
		if(digit(defaults[p]) && dbpm == 0)
		{
			dbpm = (int)defaults[p]-48;
		}
		else if(digit(defaults[p]) && dbpm < 10)
		{
			dbpm = (dbpm*10) + (int)defaults[p]-48;
		}
		else if(digit(defaults[p]) && dbpm < 100)
		{
			dbpm = (dbpm*10) + (int)defaults[p]-48;
		}
		p++;
	}
	
	write_usb_serial_blocking("\n\r",4);
	write_usb_serial_blocking(" Default Duration: \t",21);
	UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, ddur);
	write_usb_serial_blocking("\n\r",4);
	write_usb_serial_blocking(" Default Octave: \t",19);
	UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, doct);
	write_usb_serial_blocking("\n\r",4);
	write_usb_serial_blocking(" Default BPM:   \t",18);
	UARTPutDec16((LPC_UART_TypeDef *)LPC_UART0, dbpm);
	write_usb_serial_blocking("\n\r",4);

}

/*	
 *	rtttlData() is a large method which parses the data string to produce 
 *	note frequencies in Hertz and durations in seconds.
 *
 *	For each character in the array, if it is not a comma, parse it as described 
 *	inline. Hashed notes are assigned to a character with an ASCII code 7 greater 
 *	than the standard note.
 *	
 *	If a comma is found, assume that the end of the individual note description 
 *	has been found. If the duration and octave value for the note are empty, 
 *	assign the default values to it.
 *
 *	The actual frequency of the note is calculated by passing the note and octave 
 *	to music() and storing the resulting value in freq[], similarly for dur[]. Then
 *	the arrays and variables are reset and the process begins again for the next note.
 *
 *	Once finished, the value or freq[] and dur[] are printed to the terminal. Then 
 *	each note-duration pair is sent to play() to be played. When this is finished, 
 *	the DMA channel is turned off the stop the DAC outputting to the speaker.
 *	
 *	@param	str[]		The string containing the RTTTL data
 */
void rtttlData(char str[])
{	
	int q = 0;				// A counter
	char val;				// The note value
	int dot = 0;			// Flag to indicate a dotted note
	char msbDur = '0';		// Holds the most significant bit of a duration, eg 3 in 32
	char tmparray[3] = {'0','0','0'}; // [duration, note, octave]
	
	for(q=0; q<k; q++)
	{
		val = data[q];
		if(val != ',')
		{
			// If the current character is a digit and following is a letter, assume duration
			if(digit(val) && letter(data[q+1]))	
			{			
				tmparray[0] = val;
			}
			// If the current character is a digit and following is a digit, assume 2 digit duration
			else if(digit(val) && digit(data[q+1]))
			{
				msbDur = val;
			}
			// If the current character is a letter, assume note
			else if(letter(val))
			{
				tmparray[1] = val;
			}
			// If the current character is a digit, assume octave
			else if(digit(val))
			{
				tmparray[2] = val;
			}
			else if(val == '#')
			{
				tmparray[1] = tmparray[1] + 7;
			}
			else if(val == '.')
			{
				dot = 1;
			}
		}
		
		else if(val == ',')
		{
			if(tmparray[2] == '0')
			{
				tmparray[2] = (char)doct+48;		
			}
			if(tmparray[0] == '0')
			{
				tmparray[0] = (char)ddur+48;		
			}
			
			if(msbDur != '0') UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, msbDur);
			UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, tmparray[0]);
			UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, tmparray[1]);
			UARTPutChar((LPC_UART_TypeDef *)LPC_UART0, tmparray[2]);
			write_usb_serial_blocking(" ",1);
			
			freq[x] = music(tmparray[1],(int)tmparray[2]-48);
			dura[x] = duration((int)tmparray[0]-48,dot,(int)msbDur-48);	
			x++;
			
			tmparray[0] = '0';
			tmparray[1] = '0';
			tmparray[2] = '0';
			dot = 0;
			msbDur = '0';
		}
	}
	
	write_usb_serial_blocking("\n\rFrequency array values: \n\r",32);
	for(q=0; q<x; q++)
	{
		write_usb_serial_blocking(" ",1);
		UARTPutDec32((LPC_UART_TypeDef *)LPC_UART0, freq[q]*100);	
	}	
	
	write_usb_serial_blocking("\n\rDuration array values: \n\r",28);
	for(q=0; q<x; q++)
	{
		write_usb_serial_blocking(" ",1);
		UARTPutDec32((LPC_UART_TypeDef *)LPC_UART0, dura[q]*100);	
	}		
	write_usb_serial_blocking("\n\r",4);

	for(q=0;q<x;q++)	
	{
		play(freq[q],dura[q]);	// Play each note-duration pair
	}
	
	GPDMA_ChannelCmd(0, DISABLE);
	
	write_usb_serial_blocking("\n\rFinished playing \n\r",24);
}

/*	
 *	between() checks if a character is a between two values. I decided not to 
 *	use isalpha() or isdigit() from ctype.h as they require casting and I 
 *	believe this is simpler.
 *	
 *	@param	low			The lower bound
 *	@param	high		The upper bound
 *	@param	check		The value to test if it is in range
 *	@return				The 1 if a digit, 0 otherwise
 */
int between(char low, char high, char check)
{
	if ((low <= check) && (high >= check))
	{
		return 1;
	}
	return 0;
}

/*	
 *	letter() checks if a character is a letter or not.
 *	
 *	@param	test		The value to test if it is a letter
 *	@return				The 1 if a letter, 0 otherwise
 */
int letter(char test)
{
	return between('a','z', test) || between('A','Z',test);
}

/*	
 *	digit() checks if a character is a digit or not.
 *	
 *	@param	test		The value to test if it is a digit
 *	@return				The 1 if a digit, 0 otherwise
 */
int digit(char test)
{
	return between('0','9', test);
}

/*	
 *	music() provides the frequency in Hertz of the note that was parsed 
 *	from the RTTTL input string. For hashed notes (eg C#) they have 
 *	been assigned a character with an ASCII code 7 higher than the base 
 *	note (eg C) in rtttlData().
 *	
 *	@param	note		The note for which a frequency needs to be assigned
 *	@param	octave		The octave of the note
 *	@return	value		The frequency value of the requested note
 */
float music(char note, int octave)
{
	float value = 0;
	
	switch(tolower((int)note))		// Frequency value in Hz for 4th octave
	{
		case 'c':
			value = 261.63;
			break;
		case 'j':		//C#
			value = 277.18;
			break;
		case 'd':
			value = 293.66;
			break;
		case 'k':		//D#
			value = 311.13;
			break;
		case 'e':
			value = 329.63;
			break;
		case 'f':
			value = 349.23;
			break;
		case 'm':		//F#
			value = 369.99;
			break;
		case 'g':
			value = 392.00;
			break;
		case 'n':		//G#
			value = 415.30;
			break;
		case 'a':
			value = 440.00;
			break;
		case 'h':		//A#
			value = 466.16;
			break;
		case 'b':
			value = 493.88;
			break;
		case 'p':
			value = 0.00;
			break;
		default:
			value = 0.00;
			break;
	}
	
	if(octave == 5) value = value * 2;
	else if(octave == 6) value = value * 4;
	else if(octave == 7) value = value * 8;
	
	return value;
}

/*	
 *	duration() calculates the duration that the notes is required to be 
 *	played for in seconds. From the default beats per minute, it first 
 *	calculates the time in seconds each default beat is played for. Then 
 *	the duration of the current note is calculated using a formula, if 
 *	dotted this is multiplied by 1.5. 
 *	
 *	@param	dur			The raw duration value parsed from the input string
 *	@param	dot			1 if the note is dotted, 0 otherwise
 *	@param	msb			1 or 3 if the duration was 16 or 32 respectively
 *	@return	value		The duration value of the requested note in milliseconds
 */
float duration(int dur, int dot, int msb)
{	
	float blockTime = 1.0/(((float)dbpm)/60.0);
	float value = 0;
	int localDur;
	
	if(msb != 0)		localDur = (msb*10)+dur;
	else localDur = dur;
	
	value = ((float)ddur/(float)localDur)*blockTime;
	
	if(dot == 1)		value = value*1.5;
	
	return value*1000;
}

/*	
 *	play() receives a note-duration pair. It sets the timer match value to 
 *	the duration and if the note value is not 0, enables the timer and plays 
 *	the note will the interrupt flag isn't set. 
 *
 *	When it is set, the program control exits the while loop, clears the 
 *	interrupt, turns off the timer and returns to the for each loop where 
 *	the next note value is sent here. If the note value is 0, the DMA channel 
 *	is turned off for the specified time.
 *	
 *	@param	note			The frequency in Hertz of the note to be played
 *	@param	duration		The duration in milliseconds of the note
 */
void play(float note, float duration)
{
	Timer0.PrescaleOption = TIM_PRESCALE_USVAL;	// Prescale in microsecond value
	Timer0.PrescaleValue = 1000;				// 1000 us = 1 ms
	
	Match0.MatchChannel = 0;			// Match channel 0
	Match0.IntOnMatch = ENABLE;			// Interrupt on match
	Match0.StopOnMatch = ENABLE;		// Stop on match
	Match0.ResetOnMatch = ENABLE;		// Reset on match
	Match0.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;// Do nothing to external output pin when match
	Match0.MatchValue = (int)duration;
	
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &Timer0);
	TIM_ConfigMatch(LPC_TIM0, &Match0);	
	TIM_Cmd(LPC_TIM0, ENABLE);
	
	if(note == 0.00)
	{
		GPDMA_ChannelCmd(0, DISABLE);
		while(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) != SET)
		{
			//wait
		}
		GPDMA_ChannelCmd(0, ENABLE);
	}
	else
	{	
		if(prevNote == note)
		{
			GPDMA_ChannelCmd(0, DISABLE);
			delay(50);
		}
		GPDMA_ChannelCmd(0, ENABLE);
		while(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) != SET)
		{
			sine(note);
		}
	
	}
	prevNote = note;

	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	TIM_Cmd(LPC_TIM0, DISABLE);
}

