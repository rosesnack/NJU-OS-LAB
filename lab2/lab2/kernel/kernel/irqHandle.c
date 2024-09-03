#include "x86.h"
#include "device.h"

extern int displayRow;
extern int displayCol;  //准备输入的位置

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

int tail = 0;

void GProtectFaultHandle(struct TrapFrame *tf);

void KeyboardHandle(struct TrapFrame *tf);

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallGetChar(struct TrapFrame *tf);
void syscallGetStr(struct TrapFrame *tf);


void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	switch(tf->irq) {
		// TODO: 填好中断处理程序的调用
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf);
			break;
		case 0x21:
			KeyboardHandle(tf);
			break;
		case 0x80:
			syscallHandle(tf);
			break;
		default:assert(0);
	}
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}

void KeyboardHandle(struct TrapFrame *tf) {
	uint32_t code = getKeyCode();

	if (code == 0xe) { // 退格符
		//要求只能退格用户键盘输入的字符串，且最多退到当行行首
		if (displayCol > 0 && displayCol > tail) {
			displayCol--;
			bufferTail = (bufferTail - 1) % MAX_KEYBUFFER_SIZE;
			uint16_t data = 0 | (0x0c << 8); //创建一个16位的数据值，用于在屏幕上表示空白字符
			int pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
		}
	}
	else if (code == 0x1c) { // 回车符
		//处理回车情况
		keyBuffer[bufferTail] = '\n';
		bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
		if (bufferTail == bufferHead)
			bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE;
		displayRow++;
		displayCol = 0;
		tail = 0;
		if(displayRow == 25) {
			scrollScreen();
			displayRow = 24;
			displayCol = 0;
		}
	}
	else if (code < 0x81 && code != 0x3a) { 
		// TODO: 处理正常的字符
		char c = getChar(code); //defined in keyboard.c
		keyBuffer[bufferTail] = c; 
		bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
		if (bufferTail == bufferHead)
			bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE;
		
		uint16_t data = c | (0x0c << 8);
		int pos = (80 * displayRow + displayCol) * 2;
		asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));

		displayCol++;
		if (displayCol == 80) {  // 如果到达行末，转到下一行
			displayRow++;
			displayCol = 0;
			tail = 0;
			if(displayRow == 25) {  //如果屏幕满了，滚动屏幕
				scrollScreen();
				displayRow = 24;
				displayCol = 0;
			}
		}		
	}
	updateCursor(displayRow, displayCol);	
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1:
			syscallRead(tf);
			break; // for SYS_READ
		default:break;
	}
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			syscallPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct TrapFrame *tf) {
	int sel =  USEL(SEG_UDATA);
	char *str = (char*)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		// TODO: 完成光标的维护和打印到显存
		if(character == '\n') {
			displayRow++;
			displayCol = 0;
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
		}
		if (displayCol == 80) {  // 如果到达行末，转到下一行
			displayRow++;
			displayCol = 0;
			tail = 0;
			if(displayRow == 25) {  //如果屏幕满了，滚动屏幕
				scrollScreen();
				displayRow = 24;
				displayCol = 0;
			}
		}
	}
	tail = displayCol;
	updateCursor(displayRow, displayCol);
}

void syscallRead(struct TrapFrame *tf){
	switch(tf->ecx){ //file descriptor
		case 0:
			syscallGetChar(tf);
			break; // for STD_IN
		case 1:
			syscallGetStr(tf);
			break; // for STD_STR
		default:break;
	}
}

// 等按键输入完成的时候，将末尾字符通过eax寄存器传递回来
void syscallGetChar(struct TrapFrame *tf){
	// TODO: 自由实现
	enableInterrupt();
	while (bufferHead == bufferTail || keyBuffer[(bufferTail - 1) % MAX_KEYBUFFER_SIZE] != '\n')
		waitForInterrupt();
	disableInterrupt();
	tf->eax = keyBuffer[bufferHead];
	bufferHead = bufferTail;
}

void syscallGetStr(struct TrapFrame *tf){
	// TODO: 自由实现
	int sel = USEL(SEG_UDATA);   // User data/stack的段选择器
	char *str = (char*)tf->edx;
	int size = tf->ebx;
	int i = 0;
	char c = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	
	enableInterrupt();
	while(bufferHead == bufferTail || keyBuffer[(bufferTail - 1) % MAX_KEYBUFFER_SIZE] != '\n')
		waitForInterrupt();
	disableInterrupt();
	
	for (i = 0; i < size; i++) {
		if (bufferHead == bufferTail) break;
		if (keyBuffer[bufferHead] == '\n') break;
		c = keyBuffer[bufferHead];
		if (c != 0) 
			asm volatile("movb %0, %%es:(%1)"::"r"(c),"r"(str + i));
		bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE;
	}
	asm volatile("movb $0x00, %%es:(%0)"::"r"(str+i));
	bufferHead = bufferTail;
}
