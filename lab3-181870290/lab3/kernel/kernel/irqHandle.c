#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)tf;

	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf); // return
			break;
		case 0x20:
			timerHandle(tf);         // return or iret
			break;
		case 0x21:
			keyboardHandle(tf);      // return
			break;
		case 0x80:
			syscallHandle(tf);       // return
			break;
		default:assert(0);
	}

	pcb[current].stackTop = tmpStackTop;
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1:
			syscallFork(tf);
			break; // for SYS_FORK
		case 2:
			syscallExec(tf);
			break; // for SYS_EXEC
		case 3:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case 4:
			syscallExit(tf);
			break; // for SYS_EXIT
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	// TODO in lab3
	//step1 traverse pcb & descend sleepTime of blocked pcb
	for (int i = 0; i < MAX_PCB_NUM; i++)
		if (pcb[i].state == STATE_BLOCKED){
			pcb[i].sleepTime--;
			if (pcb[i].sleepTime <= 0)
				pcb[i].state = STATE_RUNNABLE;
		}

	//step2 update current process & switch conditionally
	int needSwitch = 0;
	if (pcb[current].timeCount >= MAX_TIME_COUNT || pcb[current].state != STATE_RUNNING){
		if (pcb[current].state == STATE_RUNNING){
			pcb[current].state = STATE_RUNNABLE;
			pcb[current].timeCount = 0;
		}
		for (int i = 1; i < MAX_PCB_NUM; i++){
			if (pcb[(current + i) % MAX_PCB_NUM].state == STATE_RUNNABLE){
				needSwitch = 1;
				current = (current + i) % MAX_PCB_NUM;
				pcb[current].state = STATE_RUNNING;
				pcb[current].timeCount = 0;
				break;
			}
		}
	}
	pcb[current].timeCount++;

	//step3 switch
	if (needSwitch == 1){
putChar('s');
putChar((char)(current + 48) );
putChar('\n');		
		uint32_t tmpStackTop = pcb[current].stackTop;
		pcb[current].stackTop = pcb[current].prevStackTop;
		tss.esp0 = (uint32_t) & (pcb[current].stackTop);
		asm volatile("movl %0, %%esp"::"m"(tmpStackTop));
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	}
	
	return;
}

void keyboardHandle(struct TrapFrame *tf) {
	uint32_t keyCode = getKeyCode();
	if (keyCode == 0)
		return;
	putChar(getChar(keyCode));
	keyBuffer[bufferTail] = keyCode;
	bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
	return;
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
	int sel = tf->ds; //TODO segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		if (character == '\n') {
			displayRow++;
			displayCol = 0;
			if (displayRow == 25){
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80){
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
}

void syscallFork(struct TrapFrame *tf) {
	// TODO in lab3
//putStr("fork()\n");	
	pcb[current].regs.eax = -1;
	for (int i = 0; i < MAX_PCB_NUM; i++){
		if (pcb[i].state != STATE_DEAD)
			continue;
putChar('f');
putChar((char)(i + 48));
		for (int j = 0; j < MAX_STACK_SIZE; j++)
			pcb[i].stack[j] = pcb[current].stack[j];
		for (int j = 0; j < 0x100000; j++)
			*(uint8_t *)((i + 1) * 0x100000 + j) = *(uint8_t *)((current + 1) * 0x100000 + j);
		
		pcb[i].stackTop = (uint32_t)&(pcb[i].regs);
		pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop);
		pcb[i].state = STATE_RUNNABLE;
		pcb[i].timeCount = 0;
		pcb[i].sleepTime = 0;
		pcb[i].pid = i;

		pcb[i].regs.edi = pcb[current].regs.edi;
		pcb[i].regs.esi = pcb[current].regs.esi;
		pcb[i].regs.ebp = pcb[current].regs.ebp;
		pcb[i].regs.xxx = pcb[current].regs.xxx;
		pcb[i].regs.ebx = pcb[current].regs.ebx;
		pcb[i].regs.edx = pcb[current].regs.edx;
		pcb[i].regs.ecx = pcb[current].regs.ecx;
		pcb[i].regs.irq = pcb[current].regs.irq;
		pcb[i].regs.error = pcb[current].regs.error;
		pcb[i].regs.eip = pcb[current].regs.eip;
		pcb[i].regs.eflags = pcb[current].regs.eflags;
		pcb[i].regs.esp = pcb[current].regs.esp;
		
		pcb[i].regs.cs = USEL(i * 2 + 1);
		pcb[i].regs.ss = USEL(i * 2 + 2);
		pcb[i].regs.ds = USEL(i * 2 + 2);
		pcb[i].regs.es = USEL(i * 2 + 2);
		pcb[i].regs.fs = USEL(i * 2 + 2);
		pcb[i].regs.gs = USEL(i * 2 + 2);
		
		pcb[i].regs.eax = 0;
		pcb[current].regs.eax = i;
		break;
	}
putChar('\n');
	asm volatile("int $0x20");
	return;
}

void syscallExec(struct TrapFrame *tf) {
	// TODO in lab3
	// hint: ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	int size = tf->ebx;
	int i = 0;
	char tmp[size];
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(tmp[i]):"r"(str + i));
	}
	
	uint32_t entry = 0;
	uint32_t ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	
	if (ret == -1) return;
	tf->eip = entry;
	return;
}

void syscallSleep(struct TrapFrame *tf) {
	// TODO in lab3
	if (tf->ecx <= 0) return;
	pcb[current].sleepTime = tf->ecx;
	pcb[current].state = STATE_BLOCKED;
	asm volatile("int $0x20");
	return;
}

void syscallExit(struct TrapFrame *tf) {
	// TODO in lab3
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
	return;
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
