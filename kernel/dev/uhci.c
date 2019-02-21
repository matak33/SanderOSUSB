#include "../kernel.h"
#define R_USBCMD 0x00
#define R_USBSTS 0x02
#define R_USBINT 0x04
#define R_FRNUMB 0x06
#define R_BSEADD 0x08
#define R_SOFMOD 0x0C
#define R_PORTS1 0x10
#define R_PORTS2 0x12
#define R_LEGACY 0xC0

unsigned long uhcibase = 0x0;
unsigned long uhciqueue[1024];

unsigned char uhci_is_halted(){
	return inportb(uhcibase+R_USBSTS) & 0b0000000000100000;
}

void uhci_regdump(){
	printf("UHCI: printing default values for every UHCI register:\n");
	printf("UHCI: USBCMD register, 16 bits, expected value is 0x00 , real value is : 0x%x \n", inportw(uhcibase+R_USBCMD));
	printf("UHCI: USBSTS register, 16 bits, expected value is 0x00 , real value is : 0x%x \n", inportw(uhcibase+R_USBSTS));
	printf("UHCI: USBINT register, 16 bits, expected value is 0x00 , real value is : 0x%x \n", inportw(uhcibase+R_USBINT));
	printf("UHCI: FRNUMB register, 16 bits, expected value is 0x00 , real value is : 0x%x \n", inportw(uhcibase+R_FRNUMB));
	printf("UHCI: BSEADD register, 32 bits, expected value is 0x00 , real value is : 0x%x \n", inportl(uhcibase+R_BSEADD));
	printf("UHCI: SOFMOD register, 08 bits, expected value is 0x40 , real value is : 0x%x \n", inportb(uhcibase+R_SOFMOD));
	printf("UHCI: PORTS1 register, 16 bits, expected value is 0x80 , real value is : 0x%x \n", inportw(uhcibase+R_PORTS1));
	printf("UHCI: PORTS2 register, 16 bits, expected value is 0x80 , real value is : 0x%x \n", inportw(uhcibase+R_PORTS2));
	printf("UHCI: LEGACY register, 16 bits, expected value is 0x2000 , real value is : 0x%x \n", inportw(uhcibase+R_LEGACY));
}

void uhci_global_reset(){
	outportw(uhcibase+R_USBCMD,0b0000000000000100);
	mssleep(5);
	outportw(uhcibase+R_USBCMD,0);
}


void init_uhci(unsigned long BAR){
	uhcibase = BAR & 0b11111111111111111111111111111110;
	printf("UHCI: %x is the address assigned to UHCI\n",uhcibase);
	if(inportw(uhcibase+R_LEGACY)==0xFFFF){
		printf("UHCI: BIOS says legacy is disabled\n");
	}
	unsigned long memmor = inportl(uhcibase+R_BSEADD);
	unsigned short memchk =inportw(uhcibase+R_USBCMD) & 1;
	printf("UHCI: Tell not to use LEGACY\n");
	outportw(uhcibase+R_LEGACY,0x2000);
	
	if(uhci_is_halted()==0){
		printf("UHCI: UHCI is running while it should not!!!\n");
		outportw(uhcibase+R_USBCMD,0);
		mssleep(5);
		if(uhci_is_halted()==0){
			printf("UHCI: We are in a unexpected state!\n");
			for(;;);
		}else{
			printf("UCHI: Restored UHCI into a possition we know\n");
		}
	}else{
		memmor = 0;
	}
	
	printf("UHCI: Force reset\n");
	uhci_global_reset();
	
	for(int i = 0 ; i < 1024 ; i++){
		uhciqueue[i] =  1;
	}
	
	outportw(uhcibase+R_FRNUMB,0);
	printf("UHCI: old BSEADD= 0x%x \n",memmor);
	if(memchk==0){
//	if(memmor==0){
		outportl(uhcibase+R_BSEADD,(unsigned long)&uhciqueue);
	} else {
		for(int i = 0 ; i < 10 ; i++){
			printf("%x ",((unsigned char*)memmor)[i]);
		}
		outportl(uhcibase+R_BSEADD,memmor);
	}
	outportb(uhcibase+R_SOFMOD,0x40);
	
	outportw(uhcibase+R_PORTS1,0b0000001000000100);
  	outportw(uhcibase+R_PORTS2,0b0000001000000100);
	
	mssleep(5);
	
	outportw(uhcibase+R_PORTS1,0b0000000000000100);
  	outportw(uhcibase+R_PORTS2,0b0000000000000100);
	
	mssleep(5);
	
	printf("UHCI: lets go!\n");
	outportw(uhcibase+R_USBCMD,inportw(uhcibase+R_USBCMD) | 1);
	
	sleep(2);
	
	if( inportw(uhcibase+R_USBSTS) & 0b0000000000010000){
		setLeds(1,1,1);
		printf("UHCI: Host controller process error\n");
		for(;;);
	}
	
	if( inportw(uhcibase+R_USBSTS) & 0b0000000000001000){
		setLeds(1,1,1);
		printf("UHCI: Host system error\n");
		for(;;);
	}
	
	if( inportw(uhcibase+R_USBSTS) & 0b0000000000000010){
		setLeds(1,1,1);
		printf("UHCI: Error interrupt\n");
		for(;;);
	}
	
	sleep(2);
	
	uhci_regdump();
	
	
	if(inportw(uhcibase+R_PORTS1)&0b0000000000000011){setLeds(1,0,1);printf("UHCI: PORT1 detected\n");for(;;);}
	if(inportw(uhcibase+R_PORTS2)&0b0000000000000011){setLeds(1,0,1);printf("UHCI: PORT2 detected\n");for(;;);}
}
