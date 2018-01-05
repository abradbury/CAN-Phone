/*	
 * Based on the example code provided by the university
 */
 
void delay (unsigned int tick);
int read_usb_serial_none_blocking(char *buf,int length);
int write_usb_serial_blocking(char *buf,int length);
void serial_init(void);
