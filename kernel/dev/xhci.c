#include "../kernel.h"

unsigned long xhci_base_addr = 0;
#define USBCMD 0x00000000
#define USBSTS 0x00000004
#define USBPGE 0x00000008

void init_xhci(int bus,int slot,int function){
	
	//
	// Getting base knowledge
	printf("\n\nXHCI: initialising XHCI with BUS:%x SLOT:%x FUNC:%x\n",bus,slot,function);
	int capabilityoffset = pciConfigReadWord(bus,slot,function,0x34) & 0x00FF;
	unsigned long base0 = getBARaddress(bus,slot,function,0x10);
	unsigned long base1 = getBARaddress(bus,slot,function,0x14);
	unsigned long base2 = getBARaddress(bus,slot,function,0x18);
	unsigned long base3 = getBARaddress(bus,slot,function,0x1C);
	unsigned long base4 = getBARaddress(bus,slot,function,0x20);
	unsigned long base5 = getBARaddress(bus,slot,function,0x24);
	printf("XHCI: known reg: cap= 0x%x bas0=0x%x bas1= 0x%x bas2= 0x%x bas3= 0x%x bas4= 0x%x bas5= 0x%x\n",capabilityoffset,base0,base1,base2,base3,base4,base5);
	for(int i = 0 ; i < 500 ; i++){
		printf(" %x",((unsigned long*) base0)[i]);
	}
//	xhci_base_addr = base0+0x20;
//	printf("XHCI: operational register base= 0x%x\n",xhci_base_addr);
//	unsigned long usbcmdreg = ((unsigned long*) xhci_base_addr+USBCMD)[0];
//	printf("XHCI: default value of USBCMD is 0x%x \n",usbcmdreg);
//	unsigned long usbstsreg = ((unsigned long*) xhci_base_addr+USBSTS)[0];
//	printf("XHCI: default value of USBSTS is 0x%x \n",usbstsreg);
//	unsigned long usbpgereg = ((unsigned long*) xhci_base_addr+USBPGE)[0];
//	printf("XHCI: default value of USBPGE is 0x%x \n",usbpgereg);
}
