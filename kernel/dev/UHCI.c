#include "../kernel.h"

unsigned int uhci_locs[10];
unsigned int uhci_locs_cnt = 0;

void uhci_init(int bus,int slot,int function){
	unsigned int ioAddr = getBARaddress(bus,slot,function,0x20) & 0b11111111111111111111111111111110;
	uhci_locs[uhci_locs_cnt++] = ioAddr;
	printf("--> finished\n");
	getch();
}
