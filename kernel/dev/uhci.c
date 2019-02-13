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
	sleep(2);
	outportw(uhcibase+R_USBCMD,inportw(uhcibase+R_USBCMD) & 0b1111111111111011);
	sleep(2);
	outportw(uhcibase+R_USBCMD,inportw(uhcibase+R_USBCMD) | 0b0000000000000010);
	sleep(2);
}

#define TD_PTR_QH                       (1 << 1)
#define TD_PTR_TERMINATE                (1 << 0)

typedef struct UhciQH
{
    volatile unsigned long head;
    volatile unsigned long element;

    // internal fields
    volatile unsigned long transfer;
//    Link qhLink;
//    u32 tdHead;
//    u32 active;
//    u8 pad[24];
} UhciQH;
UhciQH qh;


void init_uhci(unsigned long BAR){
	uhcibase = BAR & 0b11111111111111111111111111111110;
	printf("UHCI: %x is the address assigned to UHCI\n",uhcibase);
	if(inportw(uhcibase+R_LEGACY)==0xFFFF){
		printf("UHCI: BIOS says legacy is disabled\n");
	}
	printf("UHCI: Tell not to use LEGACY\n");
	outportw(uhcibase+R_LEGACY,0x2000);
	
	if(uhci_is_halted()==0){
		printf("UHCI: UHCI is running while it should not!!!\n");
		outportw(uhcibase+R_USBCMD,0);
		sleep(2);
		if(uhci_is_halted()==0){
			printf("UHCI: We are in a unexpected state!\n");
			for(;;);
		}else{
			printf("UCHI: Restored UHCI into a possition we know\n");
		}
	}
	
	printf("UHCI: Force reset\n");
	uhci_global_reset();
	
	qh.head = TD_PTR_TERMINATE;
	qh.element = TD_PTR_TERMINATE;
	qh.transfer = 0;
	for(int i = 0 ; i < 1024 ; i++){
		uhciqueue[i] =  (unsigned long)(TD_PTR_QH | (unsigned long)&qh);
	}
	
	outportw(uhcibase+R_FRNUMB,0);
	outportl(uhcibase+R_BSEADD,(unsigned long)&uhciqueue);
	outportb(uhcibase+R_SOFMOD,0x40);
	
	outportw(uhcibase+R_PORTS1,inportw(uhcibase+R_PORTS1) | 0b0000001000000000);
	outportw(uhcibase+R_PORTS2,inportw(uhcibase+R_PORTS2) | 0b0000001000000000);
	
	sleep(2);
	
	outportw(uhcibase+R_PORTS1,(inportw(uhcibase+R_PORTS1) & 0b1111110111111111) | 0b0000000000000100);
	outportw(uhcibase+R_PORTS2,(inportw(uhcibase+R_PORTS2) & 0b1111110111111111) | 0b0000000000000100);
	
	sleep(2);
	
//	printf("UHCI: lets go!\n");
//	outportw(uhcibase+R_USBCMD,inportw(uhcibase+R_USBCMD) | 1);
	
	sleep(2);
	
	if( inportw(uhcibase+R_USBSTS) & 0b0000000000011010){
		setLeds(1,1,1);
		printf("UHCI: There is a error occured\n");
		for(;;);
	}
	
	sleep(2);
	
	uhci_regdump();
	
	
	if(inportw(uhcibase+R_PORTS1)&1){setLeds(1,0,1);printf("UHCI: PORT1 detected\n");for(;;);}
	if(inportw(uhcibase+R_PORTS2)&1){setLeds(1,0,1);printf("UHCI: PORT2 detected\n");for(;;);}
}
