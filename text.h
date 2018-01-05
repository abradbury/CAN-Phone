/*	
 *	@author		abradbury
 */

void init_text(CAN_MSG_Type msg);
uint8_t* rx_text(CAN_MSG_Type msg);
void tx_text(char str[], int to, char type);
void end_text(CAN_MSG_Type msg);
