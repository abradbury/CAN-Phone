/*
 *	@author		abradbury
 */

#include "lpc17xx_can.h"		// Required due to CAN_MSG_Type below

void init_CAN_send();
void init_CAN_receive();
void send_CAN(uint32_t ident, uint8_t tpe, uint8_t datA, uint8_t datB);
void return_CAN();
void CAN_IRQHandler();
uint32_t whois(CAN_MSG_Type msg);
void pre(CAN_MSG_Type msg, char t);
void decipher(CAN_MSG_Type msg, char t);
void post (CAN_MSG_Type msg);
void TIMER1_IRQHandler();
void init_CAN();
void receiveBufferHandler();
