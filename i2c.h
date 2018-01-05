/*	
 *	@author		abradbury
 */

void init_i2c(void);
void i2c_write(unsigned char addr, unsigned char *data, int length);
int i2c_read(unsigned char addr, unsigned char *buf, int length);
