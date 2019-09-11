#include "../kernel.h"

void ohci_init(int bus,int slot,int function){
	unsigned int ioAddr = getBARaddress(bus,slot,function,0x10) & 0b11111111111111111111111111111110;
	if(ioAddr==0x00){
		printf("[OHCI] OHCI address expected but not found !\n");
	}else{
		printf("[OHCI] OHCI found at IO-addr 0x%x !\n",ioAddr);
	}
}
