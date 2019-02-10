#include "../kernel.h"
#define R_USBCMD 0x00
#define R_USBSTS 0x02
#define R_USBINT 0x04
#define R_FRNUMB 0x06
#define R_BSEADD 0x08
#define R_SOFMOD 0x0C
#define R_PORTS1 0x10
#define R_PORTS2 0x12

unsigned long uhcibase = 0x0;

void init_uhci(unsigned long BAR){
	uhcibase = BAR & 0b11111111111111111111111111111110;
	printf("UHCI: %x is the address assigned to UHCI\n",uhcibase);
	
	printf("UHCI: printing default values for every UHCI register:\n");
	printf("UHCI: USBCMD register, 16 bits, expected value is 0x00 , real value is : 0x%x \n", inportw(uhcibase+R_USBCMD));
	printf("UHCI: USBSTS register, 16 bits, expected value is 0x00 , real value is : 0x%x \n", inportw(uhcibase+R_USBSTS));
	printf("UHCI: USBINT register, 16 bits, expected value is 0x00 , real value is : 0x%x \n", inportw(uhcibase+R_USBINT));
	printf("UHCI: FRNUMB register, 16 bits, expected value is 0x00 , real value is : 0x%x \n", inportw(uhcibase+R_FRNUMB));
	printf("UHCI: BSEADD register, 32 bits, expected value is 0x00 , real value is : 0x%x \n", inportl(uhcibase+R_BSEADD));
	printf("UHCI: SOFMOD register, 08 bits, expected value is 0x40 , real value is : 0x%x \n", inportb(uhcibase+R_SOFMOD));
	printf("UHCI: PORTS1 register, 16 bits, expected value is 0x80 , real value is : 0x%x \n", inportw(uhcibase+R_PORTS1));
	printf("UHCI: PORTS2 register, 16 bits, expected value is 0x80 , real value is : 0x%x \n", inportw(uhcibase+R_PORTS2));
}
