#include "../kernel.h"

void init_xhci(int bus,int slot,int function){
	unsigned long barbase = getBARaddress(bus,slot,function,0x10);
	unsigned long HCIVERSION_addr = barbase+0x02;
	unsigned long HCSPARAMS1_addr = barbase+0x04;
	unsigned long HCSPARAMS2_addr = barbase+0x08;
	unsigned long HCSPARAMS3_addr = barbase+0x0C;
	unsigned long HCCPARAMS1_addr = barbase+0x10;
	unsigned char caplength = ((unsigned char*)barbase)[0];
	unsigned short HCIVERSION = ((unsigned short*)HCIVERSION_addr)[0];
	unsigned long HCSPARAMS1 = ((unsigned long*)HCSPARAMS1_addr)[0];
	unsigned long HCSPARAMS2 = ((unsigned long*)HCSPARAMS2_addr)[0];
	unsigned long HCSPARAMS3 = ((unsigned long*)HCSPARAMS3_addr)[0];
	unsigned long HCCPARAMS1 = ((unsigned long*)HCCPARAMS1_addr)[0];
	unsigned long operreg = barbase+caplength;
	printf("XHCI: xhci detected at %x with version %x \n",barbase,HCIVERSION);
	printf("XHCI: A=%x B=%x C=%x D=%x \n",HCSPARAMS1,HCSPARAMS2,HCSPARAMS3,HCCPARAMS1);
	if(HCCPARAMS1&1){
		printf("XHCI: panic: 64bit mode enabled\n");
	}
	unsigned long USBCMD_addr = operreg;
	unsigned long USBSTS_addr = operreg+4;
	unsigned long DCBAAP_addr = operreg+0x30;
	unsigned long CONFIG_addr = operreg+0x38;
	unsigned long USBCMD = ((unsigned long*)USBCMD_addr)[0];
	unsigned long CONFIG = ((unsigned long*)CONFIG_addr)[0];
	printf("XHCI: preforming reset... old values: %x [ %x ] \n",USBCMD,CONFIG);
	((unsigned long*)USBCMD_addr)[0] = 2;
	resetTicks();
	while(1){if(getTicks()==2){break;}}
	((unsigned long*)USBCMD_addr)[0] = 0;
	printf("XHCI: reset finished\n");
	
	unsigned long tkap = malloc(256*2);
	((unsigned long*)DCBAAP_addr)[0] = 0;
	((unsigned long*)DCBAAP_addr)[0] = tkap;
	((unsigned long*)CONFIG_addr)[0] = 5;
	((unsigned long*)USBCMD_addr)[0] = 1;
	
	unsigned long USBSTS = ((unsigned long*)USBSTS_addr)[0];
	printf("XHCI: status %x \n",USBSTS);
	resetTicks();
	while(1){if(getTicks()==5){break;}}
	for(int n = 1 ; n < 11 ; n++){
		unsigned long tx = (n-1)*0x10;
		unsigned long PORTSC_addr = operreg+0x400+tx;
		((unsigned long*)PORTSC_addr)[0] = 0b1000010000;
		resetTicks();
		while(1){if(getTicks()==3){break;}}
		((unsigned long*)PORTSC_addr)[0] = ((unsigned long*)PORTSC_addr)[0] & 0b11111111111111111111111111101111;
		resetTicks();
		while(1){if(getTicks()==3){break;}}
		unsigned long PORTSC = ((unsigned long*)PORTSC_addr)[0];
		printf("XHCI: PORTSC#%x: %x \n",n,PORTSC);
		if(PORTSC&1){
			printf("XHCI: port detected at %x \n",n);
		}
	}
	USBSTS = ((unsigned long*)USBSTS_addr)[0];
	printf("XHCI: status %x \n",USBSTS);
	for(;;);
}
