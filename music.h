/*	
 *	@author		abradbury
 */

void rtttlDecode(char str[]);
void rtttlSplit(char str[]);
void rtttlDefaults(char str[]);
void rtttlData(char str[]);
int between(char low, char high, char check);
int letter(char test);
int digit(char test);
float music(char note, int octave);
float duration(int dur, int dot, int msb);
void play(float note, float duration);
