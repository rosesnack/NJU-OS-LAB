#include "boot.h"

#define SECTSIZE 512


void bootMain(void) {
	int i = 0;
	//int phoff = 0x34; // program header offset
	int offset = 0x1000; // .text section offset
	unsigned int elf = 0x100000; // physical memory addr to load
	void (*kMainEntry)(void);
	kMainEntry = (void(*)(void))0x100000; // entry address of the program

	for (i = 0; i < 200; i++) {
		readSect((void*)(elf + i * 512), 1 + i);
	}

	// TODO: 阅读boot.h查看elf相关信息，填写kMainEntry、phoff、offset

	kMainEntry = (void(*)(void))((struct ELFHeader*)elf)->entry;
	//phoff = ((struct ELFHeader*)elf)->phoff;
	//offset = ((struct ProgramHeader *)(elf + phoff))->off;

	for (i = 0; i < 200 * 512; i++) {
		*(unsigned char *)(elf + i) = *(unsigned char *)(elf + i + offset);
	}

	kMainEntry();
}

void waitDisk(void) { // waiting for disk
	while((inByte(0x1F7) & 0xC0) != 0x40);
}

//dst是要写入的内存位置，offset是第几个扇区
void readSect(void *dst, int offset) { // reading a sector of disk
	int i;
	waitDisk();
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);

	waitDisk();
	for (i = 0; i < SECTSIZE / 4; i++) {  //每次读取4字节
		((int *)dst)[i] = inLong(0x1F0);
	}
}
