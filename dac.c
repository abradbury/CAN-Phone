/*	
 *	@author		abradbury
 * 
 *	Dac.c contains methods concerned with the DAC. It initialises the DAC, 
 *	initialises the sine wave for note output and then plays notes.
 */

#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "debug_frmwrk.h"
#include "dac.h"
#include "math.h"
#include "keypad.h"
#include "serial.h"

#define CALFREQ		25	// Calibration frequency

DAC_CONVERTER_CFG_Type	Dac;			// Struct used to initialise the DAC
GPDMA_LLI_Type 			DMA_LinkList;	// DMA Linked list structure for GPDMA
GPDMA_Channel_CFG_Type	DMA_Chan;		// GPDMA channel configuration structure

// 16 samples from sin0 to sin90 in degrees
uint32_t	sineSamples[16] = {0, 1045, 2079, 3090, 4067, 5000, 5877, 6691, 7431, 8090, 8660, 9135, 9510, 9781, 9945, 10000};
uint32_t 	sineValues[60];


/*	
 *	init_DAC() initialises the Digital-to-Analogue converter. It sets up
 *	the pins, enables the time out counter, double buffer and DMA, sets 
 *	the bias to performance and sets the DMA timeout to 7100.
 */
void init_DAC()
{
	PINSEL_CFG_Type PinCfg;
	
	PinCfg.Funcnum	 = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode	 = 0;
	PinCfg.Pinnum	 = 26;
	PinCfg.Portnum	 = 0;
	PINSEL_ConfigPin(&PinCfg);
	
	Dac.CNT_ENA		= 1;	
	Dac.DBLBUF_ENA	= 1;	
	Dac.DMA_ENA		= 1;	
	
	DAC_SetBias(LPC_DAC,0);
	DAC_ConfigDAConverterControl(LPC_DAC, &Dac);
	DAC_SetDMATimeOut(LPC_DAC, 7100);
	DAC_Init(LPC_DAC);
	
	write_usb_serial_blocking("DAC Initialised\n\r",18);
}

/*	
 *	feed_DAC() is a simple method that sends the inputted data to the
 *	DAC.
 *	
 *	@param	data			The data to be sent to the DAC
 */
void feed_DAC(uint32_t data)
{	
	DAC_UpdateValue(LPC_DAC,data);
}

/*	
 *	sineSetup() prepares the values that are to be used to produce the 
 *	sine wave outputted from the DAC. It uses basic values from the 
 *	sineSamples array. The values are increased by 512 and shifted 
 *	left by 6 bits so the peak-to-peak values are in the DAC's range 
 *	of 0-1023. It then sets up a linked list so that the DMA can continuously
 *	send the values to the DAC, producing a continuous wave whilst required.
 *	Finally it sets up the DMA to send this data.
 */
void sineSetup()
{		
	uint32_t 	i;
	
	for(i=0;i<60;i++)
	{
		if(i<=15)
		{
			sineValues[i] = 512 + 512*sineSamples[i]/10000;
			if(i==15) sineValues[i]= 1023;
		}
		else if(i<=30)
		{
			sineValues[i] = 512 + 512*sineSamples[30-i]/10000;
		}
		else if(i<=45)
		{
			sineValues[i] = 512 - 512*sineSamples[i-30]/10000;
		}
		else
		{
			sineValues[i] = 512 - 512*sineSamples[60-i]/10000;
		}
		sineValues[i] = (sineValues[i]<<6);
	}
	
	DMA_LinkList.SrcAddr = (uint32_t)sineValues;		// Source address
	DMA_LinkList.DstAddr = (uint32_t)&(LPC_DAC->DACR);	// Destination address
	DMA_LinkList.NextLLI = (uint32_t)&DMA_LinkList;		// Next LLI address. If none, set to 0
	DMA_LinkList.Control = 1<<26 | 2<<21 | 2<<18 | 60;	// DMACCxControl register
	
	GPDMA_Init();		// Initialise the General Purpose DMA controller
 
	DMA_Chan.ChannelNum 	= 0;						// Channel 0  
	DMA_Chan.TransferSize 	= 60;						// Length/size of transfer
	DMA_Chan.TransferWidth 	= 0;						// Used for M2M only
	DMA_Chan.SrcMemAddr 	= (uint32_t)(sineValues);	// The sine array in memory
	DMA_Chan.DstMemAddr 	= 0;						// Not needed as dest is not mem
	DMA_Chan.TransferType 	= GPDMA_TRANSFERTYPE_M2P;	// Memory to peripheral
	DMA_Chan.SrcConn		= 0;						// 0 as is memory
	DMA_Chan.DstConn 		= GPDMA_CONN_DAC;			// DAC
	DMA_Chan.DMALLI 		= (uint32_t)&DMA_LinkList;	// Link list structure
	GPDMA_Setup(&DMA_Chan);
}

/*	
 *	sine() takes a note value and creates a temporary value which
 *	it uses to set the DMA timeout frequency which in turn affects 
 *	the note played.
 *	
 *	@param	note		The note value to be played
 */
void sine(float note)
{
	uint32_t 	tmp;
	
	tmp = (CALFREQ*1000000)/(note*60);
	DAC_SetDMATimeOut(LPC_DAC,tmp);
	
	GPDMA_ChannelCmd(0, ENABLE);
}
