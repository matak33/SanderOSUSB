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

//
// Thanks to https://github.com/pdoane/osdev/blob/master/usb/uhci.c 
//



// ------------------------------------------------------------------------------------------------
// Limits

#define MAX_QH                          8
#define MAX_TD                          32

// ------------------------------------------------------------------------------------------------
// UHCI Controller I/O Registers

#define REG_CMD                         0x00        // USB Command
#define REG_STS                         0x02        // USB Status
#define REG_INTR                        0x04        // USB Interrupt Enable
#define REG_FRNUM                       0x06        // Frame Number
#define REG_FRBASEADD                   0x08        // Frame List Base Address
#define REG_SOFMOD                      0x0C        // Start of Frame Modify
#define REG_PORT1                       0x10        // Port 1 Status/Control
#define REG_PORT2                       0x12        // Port 2 Status/Control
#define REG_LEGSUP                      0xc0        // Legacy Support

// ------------------------------------------------------------------------------------------------
// USB Command Register

#define CMD_RS                          (1 << 0)    // Run/Stop
#define CMD_HCRESET                     (1 << 1)    // Host Controller Reset
#define CMD_GRESET                      (1 << 2)    // Global Reset
#define CMD_EGSM                        (1 << 3)    // Enter Global Suspend Resume
#define CMD_FGR                         (1 << 4)    // Force Global Resume
#define CMD_SWDBG                       (1 << 5)    // Software Debug
#define CMD_CF                          (1 << 6)    // Configure Flag
#define CMD_MAXP                        (1 << 7)    // Max Packet (0 = 32, 1 = 64)

// ------------------------------------------------------------------------------------------------
// USB Status Register

#define STS_USBINT                      (1 << 0)    // USB Interrupt
#define STS_ERROR                       (1 << 1)    // USB Error Interrupt
#define STS_RD                          (1 << 2)    // Resume Detect
#define STS_HSE                         (1 << 3)    // Host System Error
#define STS_HCPE                        (1 << 4)    // Host Controller Process Error
#define STS_HCH                         (1 << 5)    // HC Halted

// ------------------------------------------------------------------------------------------------
// USB Interrupt Enable Register

#define INTR_TIMEOUT                    (1 << 0)    // Timeout/CRC Interrupt Enable
#define INTR_RESUME                     (1 << 1)    // Resume Interrupt Enable
#define INTR_IOC                        (1 << 2)    // Interrupt on Complete Enable
#define INTR_SP                         (1 << 3)    // Short Packet Interrupt Enable

// ------------------------------------------------------------------------------------------------
// Port Status and Control Registers

#define PORT_CONNECTION                 (1 << 0)    // Current Connect Status
#define PORT_CONNECTION_CHANGE          (1 << 1)    // Connect Status Change
#define PORT_ENABLE                     (1 << 2)    // Port Enabled
#define PORT_ENABLE_CHANGE              (1 << 3)    // Port Enable Change
#define PORT_LS                         (3 << 4)    // Line Status
#define PORT_RD                         (1 << 6)    // Resume Detect
#define PORT_LSDA                       (1 << 8)    // Low Speed Device Attached
#define PORT_RESET                      (1 << 9)    // Port Reset
#define PORT_SUSP                       (1 << 12)   // Suspend
#define PORT_RWC                        (PORT_CONNECTION_CHANGE | PORT_ENABLE_CHANGE)


// TD Link Pointer
#define TD_PTR_TERMINATE                (1 << 0)
#define TD_PTR_QH                       (1 << 1)
#define TD_PTR_DEPTH                    (1 << 2)

// TD Control and Status
#define TD_CS_ACTLEN                    0x000007ff
#define TD_CS_BITSTUFF                  (1 << 17)     // Bitstuff Error
#define TD_CS_CRC_TIMEOUT               (1 << 18)     // CRC/Time Out Error
#define TD_CS_NAK                       (1 << 19)     // NAK Received
#define TD_CS_BABBLE                    (1 << 20)     // Babble Detected
#define TD_CS_DATABUFFER                (1 << 21)     // Data Buffer Error
#define TD_CS_STALLED                   (1 << 22)     // Stalled
#define TD_CS_ACTIVE                    (1 << 23)     // Active
#define TD_CS_IOC                       (1 << 24)     // Interrupt on Complete
#define TD_CS_IOS                       (1 << 25)     // Isochronous Select
#define TD_CS_LOW_SPEED                 (1 << 26)     // Low Speed Device
#define TD_CS_ERROR_MASK                (3 << 27)     // Error counter
#define TD_CS_ERROR_SHIFT               27
#define TD_CS_SPD                       (1 << 29)     // Short Packet Detect

// TD Token
#define TD_TOK_PID_MASK                 0x000000ff    // Packet Identification
#define TD_TOK_DEVADDR_MASK             0x00007f00    // Device Address
#define TD_TOK_DEVADDR_SHIFT            8
#define TD_TOK_ENDP_MASK                00x0078000    // Endpoint
#define TD_TOK_ENDP_SHIFT               15
#define TD_TOK_D                        0x00080000    // Data Toggle
#define TD_TOK_D_SHIFT                  19
#define TD_TOK_MAXLEN_MASK              0xffe00000    // Maximum Length
#define TD_TOK_MAXLEN_SHIFT             21

#define TD_PACKET_IN                    0x69
#define TD_PACKET_OUT                   0xe1
#define TD_PACKET_SETUP                 0x2d

unsigned long uhcibase = 0;
unsigned long uhcibuffer[1024];

typedef struct UhciQH
{
    volatile unsigned long head;
    volatile unsigned long element;

    // internal fields
//    UsbTransfer *transfer;
//    Link qhLink;
    unsigned long tdHead;
    unsigned long active;
    unsigned char pad[24];
} UhciQH;
UhciQH qh;

unsigned short getUSBCMD(){
	return inportw(uhcibase+R_USBCMD);
}

void setUSBCMD(unsigned short cmd){
	outportw(uhcibase+R_USBCMD,cmd);
}

void setUSBLegacy(unsigned short cmd){
	outportw(uhcibase+R_LEGACY,cmd);
}

void setUSBSTS(unsigned short cmd){
	outportw(uhcibase+R_USBSTS,cmd);
}

void setUSBSofmod(unsigned short cmd){
	outportw(uhcibase+R_SOFMOD,cmd);
}

void setUSBFrbseadd(unsigned long cmd){
	outportl(uhcibase+R_BSEADD,cmd);
}

void setUSBFrnm(unsigned short cmd){
	outportw(uhcibase+R_FRNUMB,cmd);
}

void setUSBInt(unsigned short cmd){
	outportw(uhcibase+R_USBINT,cmd);
}

void UhciPortSet(int port, unsigned short data){
    unsigned int status = inportw(port);
    status |= data;
    status &= ~PORT_RWC;
    outportw(port, status);
}

void UhciPortClr(int port, unsigned short data)
{
    int status = inportw(port);
    status &= ~PORT_RWC;
    status &= ~data;
    status |= PORT_RWC & data;
    outportw(port, status);
}

void uhci_regdump(){
	printf("UHCI: %x %x %x %x %x %x %x %x %x \n",
	inportw(uhcibase+REG_CMD),
	inportw(uhcibase+REG_STS),
	inportw(uhcibase+REG_INTR),
	inportw(uhcibase+REG_FRNUM),
	inportw(uhcibase+REG_FRBASEADD),
	inportw(uhcibase+REG_SOFMOD),
	inportw(uhcibase+REG_PORT1),
	inportw(uhcibase+REG_PORT2),
	inportw(uhcibase+REG_LEGSUP)
	);
}

void uhci_reset_port(int port){
	int gate = REG_PORT1 + port * 2;
	UhciPortSet(uhcibase + gate, PORT_RESET);
	sleep(50);
	UhciPortClr(uhcibase + gate, PORT_RESET);
	sleep(50);
	int found = 0;
	int status = 0;
	for(int i = 0 ; i < 10 ; i++){
		sleep(10);
		status = inportw(uhcibase + gate);
		if(~status & PORT_CONNECTION){
			found = 1;
			break;
		} 
		// Acknowledge change in status
		if (status & (PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE)){
		    UhciPortClr(uhcibase + gate, PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE);
		    continue;
		}

//		// Check if device is enabled
//		if (status & PORT_ENABLE){
//		    found = 1;
//		    break;
//		}

		// Enable the port
		UhciPortSet(uhcibase + gate, PORT_ENABLE);
	}
	uhci_regdump();
	if(found==1){
		printf("UHCI: GEVONDEN!!!\n");
	}
}

void uhci_probe(){
	int portcount = 5;
	for(int port = 0 ; port < portcount ; port++){
		printf("UHCI: probing port # %x \n",port);
		uhci_reset_port(port);
	}
	
}

void init_uhci(unsigned long BAR){
	
	//
	// What is the right address?
	uhcibase = BAR & 0b11111111111111111111111111111110;
	printf("UHCI: %x is the address assigned to UHCI\n",uhcibase);
	
//	//
//	// Are we already running? (for example because BIOS started us)
//	if(getUSBCMD()&1){
//		printf("UHCI: driver already running! stopping it...\n");
//		setUSBCMD(0);
//	}
//	
//	//
//	// Telling BIOS we are taking it over from here.
//	printf("UHCI: telling BIOS we take it over from here...\n");
//	setUSBLegacy(0x8f00);
//	
//	//
//	// Disable interrupts
//	printf("UCHI: disabling interrupts...\n");
//	setUSBInt(0);
//	
//	//
//	// Assign framelist
//	printf("UCHI: assign framelist...\n");
//	
//    	qh.head = TD_PTR_TERMINATE;
//    	qh.element = TD_PTR_TERMINATE;
//	for(int i = 0 ; i < 1024 ; i++){
//		uhcibuffer[i] = TD_PTR_QH | (unsigned long)&qh;
//	}
//	setUSBFrnm(0);
//	setUSBFrbseadd((unsigned long)&uhcibuffer);
//	setUSBSofmod(0x40);
//
//	//
//	// Clear status	
//	printf("UHCI: clear status...\n");
//	setUSBSTS(0xFFFF);
//	
//	//
//	// Enable controller
//	printf("UHCI: enable controller...\n");
//	setUSBCMD(1);
//	
	//
	// Probing
	uhci_probe();
	
	
	
}
