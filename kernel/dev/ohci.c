#include "../kernel.h"

void init_ohci(unsigned long BAR){
	printf("OHCI: %x is the address assigned to OHCI\n",BAR);
	unsigned long *ohciregs = (unsigned long*) BAR;
	unsigned char version = ohciregs[0] & 0b00000001111111;
	if(version!=0x10){
		printf("OHCI: invalid HCREVISION number (0x%x) \n",version);
		return;
	}else{
		printf("OHCI: HCREVISION number is like expected (0x10)\n");
	}
}
