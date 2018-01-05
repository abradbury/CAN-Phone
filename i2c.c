/*	
 *	@author		abradbury
 *	
 *	i2c.c() contains methods for initialising the I2C bus and for 
 *	reading and writing data to the bus.
 */

#include "lpc17xx_pinsel.h"
#include "serial.h"
#include "lpc17xx_i2c.h"
#include "i2c.h"

#define I2CDEV			LPC_I2C1

/*	
 *	init_i2c() initialises the I2C bus by setting the pins both for 
 *	read and write operations. It also initialises I2C1 to a clock 
 *	rate of 100000 Hz.
 */
void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Funcnum = 3;
	PinCfg.Pinnum = 0;

	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);

	PinCfg.Pinnum = 1;
	PINSEL_ConfigPin(&PinCfg);

	I2C_Init(I2CDEV, 100000);
	I2C_Cmd(I2CDEV, ENABLE);	

	write_usb_serial_blocking("I2C initialised \n\r",19);
}

/*	
 *	i2c_write() is used to write to devices on the I2C bus such as 
 *	the LCD, keypad and 7-segment display. It creates a struct and 
 *	sets its component values to those received. The data is 
 *	transfered in master polling mode. A detailed breakdown of the
 *	struct values is given inline.
 *	
 *	@param	addr			The address of the I2C device to write to
 *	@param	data			A pointer to the data to write onto the bus
 *	@param	length			The length of the data to write to the bus
 */
void i2c_write(unsigned char addr, unsigned char *data, int length)
{
	I2C_M_SETUP_Type setup;

	setup.sl_addr7bit = addr;			// The address of the device to write to                
	setup.tx_data = data;				// A pointer to the data to send	
	setup.tx_length = length;			// The length of the data to send
	setup.tx_count = 0;					// Transmit data counter
	setup.rx_data = NULL;				// A pointer to where the received data will go
	setup.rx_length = 0;				// Length of data to receive
	setup.rx_count = 0;                 // Receive data counter               
	setup.retransmissions_max = 3;		// Max number of times to retry
	setup.retransmissions_count = 0;	// A counter for the number of retries
	setup.status = 0;					// Current status of the I2C activity	

	I2C_TRANSFER_OPT_Type Opt = I2C_TRANSFER_POLLING;

	I2C_MasterTransferData(I2CDEV, &setup, Opt);	 
}

/*	
 *	i2c_read() performs reads from devices on the I2C bus such as the LCD
 *	screen, keypad and 7-segment displays. It creates a struct and sets 
 *	its values using the parameters.
 *	
 *	@param	addr		The address of the I2C device to read from
 *	@param	data		A pointer to where the read data will be written to
 *	@param	length		The length of the data to receive from the bus
 *	@return				1 if successful, 0 otherwise
 */
int i2c_read(unsigned char addr, unsigned char *buf, int length) 
{
	I2C_M_SETUP_Type data;
	
	data.sl_addr7bit = addr;	            
	data.tx_data = NULL;	
	data.tx_length = 0;	
	data.tx_count = 0;
	data.rx_data = (uint8_t*)buf;	
	data.rx_length = length;
	data.rx_count = 0;               
	data.retransmissions_max = 3;
	data.retransmissions_count = 0;
	data.status = 0;		

	I2C_TRANSFER_OPT_Type Opt = I2C_TRANSFER_POLLING;
	
	return(I2C_MasterTransferData(I2CDEV, &data, Opt));
}
