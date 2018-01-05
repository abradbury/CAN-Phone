/*	
 *	@author		P. Cooper
 * 
 *	canbus_msg.h contains macros used for the decoding and composition of messages from the 
 *	CAN network courtesy of P. Cooper. The file is mostly unchanged from the original.
 *	Modifications include adding the bounce command macro and correcting lines 64-66.
 */

// canbus_msg.h
// basic bits used in message format

// Pre defined data packet types,
#define	VOICEDATA		0x00	// Voice data packet
#define	SYSTEMDATA		0x01	// System data
#define	SMSDATA			0x02	// Text message data
#define	MMSDATA			0x03	// Multimedia data

#define	CMD_WHOIS		0x00	// Who is on line, sent to the broardcast address
								// stations return with an CMD_IAM message
#define	CMD_IAM			0x08	// Tell other net users who I am, sent after a CMD_WHOIS
								// message has been sent out by a station, each station then
								// waits the station ID time in mS, before transmitting i'm 
								// here, 
#define	CMD_VOICE		0x03	// voice data on net	
#define	CMD_CHECKSUM	0x04	// passing a simple checksum over the network			
#define	CMD_DNS			0x01	// Send a phone number request out to the net, getting
								// to the CANADD_DNS machine, shoule return a CMD_CALLID
								// message, with a call slot on the network
#define	CMD_CALLID		0x02	// returning network call slot from the CANADD_DNS machine
#define	CMD_CLEARCALL	0x07	// clear a current call ID allocated from the network,
								// needs to be sent to the CANADD_DNS machine

#define	CMD_DIALTONES	0x05	// return a list of dial tones available from the server
								// to the caller, using a CMD_TEXTBLOCK message
				
#define	CMD_STEXT		0x09	// Start of a text block, tell user the buffer size needed
								// to receive the data in bytes 0 and 1
#define CMD_TEXTBLOCK	0x06	// block of text sent to user, In the packet ID there is an
								// eight bit count of block number, has to be sent after
								// an CMD_STEXT, block size will tell the receiving end
								// how many packets to expect
#define	CMD_ETEXT		0x0a	// End of text block, so the remote end know's
#define	CMD_ERROR		0x0b	// Error message send down the bus
#define	CMD_TESTSOUND	0x0c	// send a test sound string over the bus :-)
#define	CMD_BOUNCE		0x0d	// bounce


// Predefined network addresses used on the can bus
#define	CANADD_DNS		0x01		// six bit address to the DNS machine
#define CANADD_GW		0x01		// six bit address to gateway machine

// Pre defined Bit Masks
#define	CAN2BIT			0x00000003			// 2 bit mask
#define	CAN4BIT			0x0000000f			// 4 bit mask
#define	CAN6BIT			0x0000003f			// 6 bit mask
#define	CAN8BIT			0x000000ff			// 8 bit mask
// Pre defined Bit shifts
#define	CANSHIFT_TARGET		0
#define	CANSHIFT_SOURCE		6
#define	CANSHIFT_CMD		12
#define	CANSHIFT_CALLID		18
#define	CANSHIFT_COUNT		18
#define	CANSHIFT_TYPE		26
// Pre defined macros
#define	CAN_GET_TARGET_ADD(a)	(a & CAN6BIT)					// crunch put the target address from header
#define	CAN_GET_SOURCE_ADD(a)	((a>>CANSHIFT_SOURCE) & CAN6BIT)// crunch out the source address from the header
#define	CAN_GET_CMD(a)		((a>>CANSHIFT_CMD) & CAN6BIT)		// crunch out the command wanted from the header
#define	CAN_GET_CALLID(a)	((a>>CANSHIFT_CALLID) & CAN4BIT)	// crunch out the CALLID from the header
#define	CAN_GET_COUNT(a)	((a>>CANSHIFT_COUNT) & CAN8BIT)		// crunch out the block count from the header
#define	CAN_GET_TYPE(a)		((a>>CANSHIFT_TYPE) & CAN2BIT)		// crunch out the data type from the header

