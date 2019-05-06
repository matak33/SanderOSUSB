#include "../kernel.h"

typedef struct EhciCapRegs
{
    unsigned char capLength;
    unsigned char reserved;
    unsigned short hciVersion;
    unsigned long hcsParams;
    unsigned long hccParams;
}EhciCapRegs;

unsigned long ehci_base = 0;

void ehci_stop(){
	((unsigned long*)ehci_base)[0] = 0;
	resetTicks();
	while(!((unsigned long*)ehci_base+4)[0]&0x00001000){if(getTicks()==10){printf("ECHI: Timeout\n");break;}}
}

void ehci_reset(){
	((unsigned long*)ehci_base)[0] = 0b00000000000000000000000000000010;
	resetTicks();
	while(((unsigned long*)ehci_base)[0]&0b10){if(getTicks()==10){printf("ECHI: Timeout\n");break;}}
}

void ehci_regdump(){
	printf("EHCI: USBCMD:           %x \n",((unsigned long*)ehci_base+0x00)[0]);
	printf("EHCI: USBSTS:           %x \n",((unsigned long*)ehci_base+0x04)[0]);
	printf("EHCI: USBINTR:          %x \n",((unsigned long*)ehci_base+0x08)[0]);
	printf("EHCI: FRINDEX:          %x \n",((unsigned long*)ehci_base+0x0C)[0]);
	printf("EHCI: CTRLDSSEGMENT:    %x \n",((unsigned long*)ehci_base+0x10)[0]);
	printf("EHCI: PERIODICLISTBASE: %x \n",((unsigned long*)ehci_base+0x14)[0]);
	printf("EHCI: ASYNCLISTADDR:    %x \n",((unsigned long*)ehci_base+0x18)[0]);
	printf("EHCI: CONFIGFLAG:       %x \n",((unsigned long*)ehci_base+0x40)[0]);
	for(int i = 1 ; i < 10 ; i++){
		printf("EHCI: PORTSC%x:          %x \n",i,((unsigned long*)ehci_base+(0x44 + (4*i-1)))[0]);
	}
}

#define HCCPARAMS_EECP_MASK             (255 << 8)

unsigned long framelist[1024];

void init_ehci(int bus,int slot,int function){
	unsigned long BAR = getBARaddress(bus,slot,function,0x10);
	ehci_base = BAR;//>>8;
	EhciCapRegs stu = (EhciCapRegs)((EhciCapRegs*) ehci_base)[0];
	printf("ECHI: detected at bar %x \n",ehci_base);
	unsigned char capreglen = ((unsigned char*)ehci_base)[0];
	printf("EHCI: capability pointer length %x \n",capreglen);
	ehci_base  = ehci_base + capreglen;
	
	printf("EHCI: value of ehci-cmd is %x \n",((unsigned long*)ehci_base)[0]);
	
	printf("EHCI: capability register explain CAPLEN:%x RESERV:%x HCIVER:%x HCSPAR:%x HCCPAR:%x \n",stu.capLength,stu.reserved,stu.hciVersion,stu.hcsParams,stu.hccParams);
	
	printf("EHCI: we have %x ports!\n",stu.hcsParams & 0b01111);
	unsigned char capext = (stu.hccParams & 0b1111111100000000) >> 8;
	printf("EHCI: we have biosprobe %x !\n",capext);
	// stop controller
	ehci_stop();
	printf("ECHI: stopped! \n");
	
	// reset controller
	ehci_reset();
	printf("ECHI: reset! \n");
	
	// regdump
	ehci_regdump();
	
	//&framelist;
	
	// end
	printf("EHCI: set default status finished!");
	
	//for(;;);
}
