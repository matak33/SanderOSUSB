#include "../kernel.h"

//
// REGSET

// 00h USBCMD USB Command Core 2.3.1
#define USBCMD 0x00

// 04h USBSTS USB Status Core 2.3.2
#define USBSTS 0x04

// 08h USBINTR USB Interrupt Enable Core 2.3.3
#define USBINTR 0x08

// 0Ch FRINDEX USB Frame Index Core 2.3.4
#define FRINDEX 0x0C

// 10h CTRLDSSEGMENT 4G Segment Selector Core 2.3.5
#define CTRLDSSEGMENT 0x10

// 14h PERIODICLISTBASE Frame List Base Address Core 2.3.6
#define PERIODICLISTBASE 0x14

// 18h ASYNCLISTADDR Next Asynchronous List Address Core 2.3.7
#define ASYNCLISTADDR 0x18

// 1C-3Fh Reserved 40h CONFIGFLAG Configured Flag Register Aux 2.3.8
// 44h PORTSC(1-N_PORTS) Port Status/Control Aux 2.3.9

void init_ehci(unsigned long BAR){
	printf("EHCI: ehci detected at BAR %x !\n",BAR);
	printf("EHCI: usbcmd: %x \n",inportl(BAR));
	for(;;);
}
