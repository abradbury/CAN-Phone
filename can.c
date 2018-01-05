/*	
 *	@author		abradbury
 * 
 *	Can.c initialises the CAN bus, prepares the board to receive and send 
 *	messages over the CAN network, deciphers and deals with incoming 
 *	messages (excluding text and RTTTL) and creates and utilises a received 
 *	messages buffer.
 */

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "canbus_msg.h"
#include "lpc17xx_timer.h"
#include "serial.h"	
#include "can.h"
#include "lcd.h"
#include "text.h"
#include "debug_frmwrk.h"
#include "sevenseg.h"
#include "mysys.h"

#define CAN		LPC_CAN2
#define IAM		0x14008440			// I am online from bench 07 to 0

CAN_MSG_Type		SMsg;			// Stores the message to be sent
CAN_MSG_Type		RMsg;			// Stores the message to be received
CAN_MSG_Type 		buffer[255];	// Buffer for storing messages, assume initialised to 0/empty
int					bufMsgs = 0;	// Holds the number of messages buffered
int					decMsgs = 0;	// Holds the number of messages deciphered
int					textCount = 0;	// A counter for the number of text blocks received
uint32_t 			whoID = 0x14008440;	// Message ID template for whoIs response
uint8_t 			whoTarget;		// Holds the target address for the reply
CAN_MSG_Type		who;			// Stores the received whoIs message for later response 
TIM_TIMERCFG_Type	Timer;			// The timer struct for the whoIs timer
TIM_MATCHCFG_Type	Match;			// The match struct for the whoIs timer

/*	
 *	init_CAN_send() sets up the station to send messages over the CAN network 
 *	by setting the board pins and initialising a template message.
 */
void init_CAN_send()
{
	SMsg.format	= EXT_ID_FORMAT;	
	SMsg.id		= 0;	
	SMsg.len	= 8;	
	SMsg.type	= DATA_FRAME;
	SMsg.dataA[0] = SMsg.dataA[1] = SMsg.dataA[2] = SMsg.dataA[3] = 0x00;
	SMsg.dataB[0] = SMsg.dataB[1] = SMsg.dataB[2] = SMsg.dataB[3] = 0x00;

	PINSEL_CFG_Type PinCfgT;
	PinCfgT.Funcnum 	= 2;
	PinCfgT.OpenDrain	= 0;	
	PinCfgT.Pinmode 	= 0;		
	PinCfgT.Pinnum		= 5;
	PinCfgT.Portnum 	= 0;		
	PINSEL_ConfigPin(&PinCfgT);
}

/*	
 *	init_CAN_receive() prepares the station to receive messages from the CAN 
 *	network by setting the board's pins and initialising a template message.
 */
void init_CAN_receive()
{
	RMsg.format	= 0x00;
	RMsg.id		= 0x00;
	RMsg.len	= 0x00;
	RMsg.type	= 0x00;
	RMsg.dataA[0] = RMsg.dataA[1] = RMsg.dataA[2] = RMsg.dataA[3] = 0x00;
	RMsg.dataB[0] = RMsg.dataB[1] = RMsg.dataB[2] = RMsg.dataB[3] = 0x00;
	
	PINSEL_CFG_Type PinCfgR;
	PinCfgR.Funcnum 	= 2;
	PinCfgR.OpenDrain 	= 0;	
	PinCfgR.Pinmode 	= 0;		
	PinCfgR.Pinnum 		= 4;
	PinCfgR.Portnum 	= 0;		
	PINSEL_ConfigPin(&PinCfgR);
}

/*	
 *	send_CAN() is used to send a message over the network. It can be customised 
 *	using several input parameters and uses the return from CAN_SendMsg to inform 
 *	the user whether the packet has been sent or not.
 *	
 *	@param	ident		The 29 bit identifier for the message
 *	@param	tpe			The type (Data or Remote Frame) to transmit the message as
 *	@param	datA		The 4 bytes of data for data array A 
 *	@param	datB		The 4 bytes of data for data array B
 */
void send_CAN(uint32_t ident, uint8_t tpe, uint8_t datA, uint8_t datB)
{	
	SMsg.format	= EXT_ID_FORMAT;	
	SMsg.id		= ident;	
	SMsg.len	= 8;	
	SMsg.type	= tpe;
	SMsg.dataA[0] = SMsg.dataA[1] = SMsg.dataA[2] = SMsg.dataA[3] = datA;
	SMsg.dataB[0] = SMsg.dataB[1] = SMsg.dataB[2] = SMsg.dataB[3] = datB;
	
	if(CAN_GET_CMD(RMsg.id) == CMD_IAM)
	{
		CAN_SendMsg(CAN, &SMsg);	// Do not decipher
		//write_usb_serial_blocking("I am\n\r",6);
	}
	else
	{
		if(CAN_SendMsg(CAN, &SMsg) == SUCCESS) decipher(SMsg,'s');
		else write_usb_serial_blocking("Message not sent\n\r",18);
	}
}

/*	
 *	return_CAN() receives a message from the buffer, stores it in RMsg (the 
 *	template message) and passes it to the decipher method for decoding.
 */
void return_CAN()
{
	if(CAN_ReceiveMsg (CAN, &RMsg) == SUCCESS) decipher(RMsg,'r');
	else write_usb_serial_blocking("Message not recieved\n\r",23);		
}

/*	
 *	CAN_IRQHandler() is triggered when a message is received from the CAN 
 *	network. The message is received from the receive buffer and stored in 
 *	the buffer. The counter is then incremented and the 4 are LED's turned 
 *	on to indicate a received message. 
 *
 *	If a received message is a who is online command, the whois() method
 *	is called.
 *	
 *	To ensure that messages that are sent rapidly over the network can be 
 *	received reliably, every message is buffered, not just text and RTTTL 
 *	messages.
 */
void CAN_IRQHandler()
{	
	CAN_ReceiveMsg (CAN, &RMsg);	
	
	if(CAN_GET_CMD(RMsg.id) == CMD_WHOIS) whois(RMsg);

	buffer[bufMsgs] = RMsg;
	bufMsgs++;
	
	GPIO_SetDir(1, 0x00B40000, 1);
	GPIO_SetValue(1, 0x00B40000);
}

/*	
 *	whois() is called when a 'who is online?' command is received from the 
 *	CAN network. It starts the timer and then 'ors' in the return address
 *	to the template ID 'whoID'. When the timer is up, a message with this
 *	ID is sent.
 *	
 *	@param	msg			The who is message received.
 *	@return	whoID		The ID of the reply message.
 */
uint32_t whois(CAN_MSG_Type msg)
{	
	TIM_Cmd(LPC_TIM1, ENABLE);
	
	whoTarget = CAN_GET_SOURCE_ADD(msg.id);
	whoID = whoID | whoTarget;
	return whoID;
}

/*	
 *	pre() prints out the the terminal the precursor information about the 
 *	message. It was separated from the decipher() method to allow greater 
 *	flexibility. For example, when receiving a text message, no 
 *	information is need, so this is not called.
 *	
 *	@param	msg			The message received.
 *	@param	t			A flag indicating is a message is being received
 *						or sent. 'r' if receiving, 's' if sending.
 */
void pre(CAN_MSG_Type msg, char t)
{
	if(t == 'r')
	{
		write_usb_serial_blocking("- Message received: (",21);
		UARTPutDec16((LPC_UART_TypeDef *)LPC_UART0, bufMsgs);
		write_usb_serial_blocking("/",1);
		UARTPutDec16((LPC_UART_TypeDef *)LPC_UART0, decMsgs);
		write_usb_serial_blocking(")\n\r",4);
	}
	else if(t == 's')
	{
		write_usb_serial_blocking("- Message sent:\n\r",19);
		clear_screen();
	}
}

/*	
 *	decipher() is the main method that deals with the received messages, 
 *	though it can also be used for sent messages. It uses the message's 
 *	ID and matches it to to the available commands, then executes certain 
 *	instructions depending on the type of message. It is also able to deal 
 *	with erroneous commands and prints out the message's ID to enable debugging.
 *	
 *	@param	msg			The message received.
 *	@param	t			A flag indicating is a message is being received
 *						or sent. 'r' if receiving, 's' if sending.
 */
void decipher(CAN_MSG_Type msg, char t)
{
	switch(CAN_GET_CMD(msg.id))
	{
		case CMD_WHOIS:
			if(t == 'r')	 
			{
				// Any buffered who is messages have already been dealt
				// with, so this just prints out the message.
				pre(msg, t);
				write_usb_serial_blocking("Who is?",7);
				post(msg);
			}
			break;
		case CMD_DNS:
			pre(msg, t);
			write_usb_serial_blocking("Name Lookup",11);
			post(msg);
			break;
		case CMD_CALLID:
			pre(msg, t);
			write_usb_serial_blocking("Name Lookup data",16);
			post(msg);
			break;
		case CMD_VOICE:
			pre(msg, t);
			write_usb_serial_blocking("Voice",5);
			break;
		case CMD_CHECKSUM:
			pre(msg, t);
			write_usb_serial_blocking("Checksum",8);
			post(msg);
			break;
		case CMD_DIALTONES:
			pre(msg, t);
			write_usb_serial_blocking("Ringtone List",13);
			post(msg);
			break;
		case CMD_STEXT:
			pre(msg, t);
			write_usb_serial_blocking("Start of text block",19);
			init_text(msg);		// Setup array for forthcoming data
			textCount = 0;
			post(msg);
			break;
		case CMD_TEXTBLOCK:
			textCount++;
			rx_text(msg);
			//UARTPuts((LPC_UART_TypeDef *)LPC_UART0, msg.dataA);	// Don't need dataB
			break;
		case CMD_CLEARCALL:	
			pre(msg, t);
			write_usb_serial_blocking("Clear call ID",13);
			post(msg);
			break;
		case CMD_IAM:
			pre(msg, t);
			write_usb_serial_blocking("I am online",11);
			post(msg);
			break;
		case CMD_ETEXT:
			write_usb_serial_blocking(" ",1);
			UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, textCount);
			write_usb_serial_blocking("\n\n\r",4);
			pre(msg, t);
			write_usb_serial_blocking("End of text block",17);
			end_text(msg);
			post(msg);
			write_usb_serial_blocking("******\n\r",10);
			break;
		case CMD_ERROR:
			pre(msg, t);
			write_usb_serial_blocking("Error on bus",12);
			post(msg);
			break;
		case CMD_TESTSOUND:
			pre(msg, t);
			write_usb_serial_blocking("Sound test",10);
			break;
		case CMD_BOUNCE:
			pre(msg, t);
			write_usb_serial_blocking("Bounce",6);
			post(msg);
			break;
		default:
			pre(msg, t);
			write_usb_serial_blocking("Unknown command (",17);
			UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, CAN_GET_CMD(msg.id));
			write_usb_serial_blocking(")",1);
			post(msg);
			break;
	}
}

/*	
 *	post() prints out the post message information to the terminal. 
 *	When printing out the to and from addresses of a message, it also 
 *	outputs these to the 7-segment display.
 *	
 *	@param	msg			The message received.
 */
void post (CAN_MSG_Type msg)
{
	write_usb_serial_blocking(" from station ",14);
	UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, CAN_GET_SOURCE_ADD(msg.id));
	seg_digit(CAN_GET_SOURCE_ADD(msg.id),1);
	write_usb_serial_blocking(" to station ",12);
	UARTPutDec((LPC_UART_TypeDef *)LPC_UART0, CAN_GET_TARGET_ADD(msg.id));
	seg_digit(CAN_GET_TARGET_ADD(msg.id),2);
	write_usb_serial_blocking("\n\r",4);

	write_usb_serial_blocking("Message id: \t",13);
	UARTPutHex32((LPC_UART_TypeDef *)LPC_UART0, msg.id);
	write_usb_serial_blocking("\t",2);
	
	write_usb_serial_blocking("Data: \t",8);
	UARTPuts((LPC_UART_TypeDef *)LPC_UART0, msg.dataA);
	write_usb_serial_blocking("\n\r",4);
	write_usb_serial_blocking("\n\r",4);	
}

/*	
 *	TIMER1_IRQHandler() is called when the timer started when a 'who is 
 *	online' command is received reaches the value in the match register.
 *	For desk 7 this is after 17ms. When this method is called, an 'I am
 *	online' message is sent to the 'who is' sender.
 */
void TIMER1_IRQHandler()
{
	TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
	send_CAN(whois(who), DATA_FRAME, 0x00, 0x00);
}

/*	
 *	init_CAN() sets up the CAN bus pins and enables it using GPIO, initialises 
 *	the timer and match register for the 'who is online?' delayed response (more 
 *	information given inline), calls the CAN send and receive initialiser 
 *	methods, enables the CAN and Timer1 interrupts and sets up the memory for 
 *	dynamic array allocation.
 */
void init_CAN()
{
	PINSEL_CFG_Type PinCfg;
	PinCfg.Funcnum 		= 0;
	PinCfg.OpenDrain 	= 0;	
	PinCfg.Pinmode 		= 0;		
	PinCfg.Pinnum 		= 10;
	PinCfg.Portnum 		= 0;		
	PINSEL_ConfigPin(&PinCfg);
	
	GPIO_SetDir(0,0x00000400,1);	
	GPIO_SetValue(0,0x00000000);
	
	CAN_Init(CAN, 250000);
	CAN_ModeConfig(CAN, CAN_OPERATING_MODE, ENABLE);
	
	Timer.PrescaleOption = TIM_PRESCALE_USVAL;	// Prescale in microsecond value
	Timer.PrescaleValue = 1000;					// 1000 us = 1 ms
	
	Match.MatchChannel 	= 1;					// Match channel 1
	Match.IntOnMatch 	= ENABLE;				// Interrupt on match
	Match.StopOnMatch 	= ENABLE;				// Stop on match
	Match.ResetOnMatch 	= DISABLE;				// Do not reset on match
	Match.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;// Do nothing to external output pin when match
	Match.MatchValue 	= 17;
	
	TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &Timer);
	TIM_ConfigMatch(LPC_TIM1, &Match);
	
	init_CAN_send();
	init_CAN_receive();	
	
	CAN_IRQCmd(CAN, CANINT_RIE, ENABLE);		// CAN Receiver Interrupt Enable
	CAN_SetAFMode(LPC_CANAF,CAN_AccBP);			// Acceptance Bypass Filter
	NVIC_EnableIRQ(CAN_IRQn);					// CPU CAN Interrupt Enable
	NVIC_EnableIRQ(TIMER1_IRQn);				// CPU Timer Interrupt Enable	

	MSYS_Init ((void*) 0x2007C000, 0x4000);

	write_usb_serial_blocking("CAN initialised\n\r",19);
}

/*	
 *	receiveBufferHandler() is the main method dealling with the receive buffer.
 *	While the number of deciphered messages isn't equal to the number of 
 *	messages currently in the buffer, the buffered messages are deciphered.
 *	Then, when all the buffered messages have been decoded, the counters are 
 *	reset and then LEDs (turned on when a message is received) are turned off.
 */
void receiveBufferHandler()
{
	while(decMsgs != bufMsgs)
	{
		if(bufMsgs > 0)
		{
			decipher(buffer[decMsgs],'r');			
			decMsgs++;
		}
	}
	if((decMsgs == bufMsgs) && (bufMsgs != 0))
	{
		bufMsgs = 0;
		decMsgs = 0;

		GPIO_ClearValue(1, 0x00B40000);
	}
}
