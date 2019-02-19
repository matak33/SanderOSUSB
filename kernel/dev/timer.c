#include "../kernel.h"

//
// TIMER
//
//

extern void timerirq();

int clock = 0;
volatile int ticks = 0;
volatile int msticks = 0;

void sleep(unsigned int time){
	resetTicks();
	while(((unsigned int)getTicks())!=time);
}

void mssleep(unsigned int time){
	resetmsticks();
	while(((unsigned int)getmsticks())!=time);
}

int getmsticks(){
	return msticks;
}

int getTicks(){
	return ticks;
}

void resetmsticks(){
	msticks = 0;
}

void resetTicks(){
	ticks = 0;
}

void irq_timer(){
	clock++;
	outportb(0x20,0x20);
	msticks++;
	if(clock % 0x18 == 0){
		unsigned char* vidmem = (unsigned char*)0xb8000;
		ticks++;
		if(vidmem[0]=='-'){
			vidmem[0]='\\';
		}else if(vidmem[0]=='\\'){
			vidmem[0]='|';
		}else if(vidmem[0]=='|'){
			vidmem[0]='/';
		}else if(vidmem[0]=='/'){
			vidmem[0]='-';
		}else{
			vidmem[0]='-';
		}
	}
}

void init_timer(){ // 10ms / tick
	int divisor = 1193180 / 100;       /* Calculate our divisor */
    	outportb(0x43, 0x36);             /* Set our command byte 0x36 */
    	outportb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    	outportb(0x40, divisor >> 8);     /* Set high byte of divisor */
    	setNormalInt(0,(unsigned long)timerirq);
}

