#include "../kernel.h"

void init_uhci(int bus,int slot,int function){
	unsigned long BAR = getBARaddress(bus,slot,function,0x20);
	printf("UHCI: found at %x \n",BAR);
	outportw(BAR,0);
	printf("UHCI: stopped!\n");
}
