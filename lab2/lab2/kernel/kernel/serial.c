#include "x86.h"
#include "device.h"

void initSerial(void) {
	outByte(SERIAL_PORT + 1, 0x00);
	outByte(SERIAL_PORT + 3, 0x80);
	outByte(SERIAL_PORT + 0, 0x01);
	outByte(SERIAL_PORT + 1, 0x00);
	outByte(SERIAL_PORT + 3, 0x03);
	outByte(SERIAL_PORT + 2, 0xC7);
	outByte(SERIAL_PORT + 4, 0x0B);
}

static inline int serialIdle(void) {
	return (inByte(SERIAL_PORT + 5) & 0x20) != 0;
}

void putChar(char ch) {
	while (serialIdle() != TRUE);
	outByte(SERIAL_PORT, ch);
}

void putStr(char *ch){
	while(ch && (*ch) && (*ch)!='\0'){
		putChar(*ch);
		ch++;
	}
}

void putNum(int num){
    if (num == 0){ putChar('0'); return;}
    if (num < 0){ putChar('-'); num = -num;}
    while(num){
        char ch = (num % 10) + '0';
        putChar(ch);
        num /= 10;
    }
}


