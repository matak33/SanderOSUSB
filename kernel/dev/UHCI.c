#include "../kernel.h"


typedef struct UsbController
{
    struct UsbController *next;
    void *hc;

    void (*poll)(struct UsbController *controller);
} UsbController;
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

// ------------------------------------------------------------------------------------------------
// Transfer Descriptor

typedef struct UhciTD
{
    volatile unsigned long link;
    volatile unsigned long cs;
    volatile unsigned long token;
    volatile unsigned long buffer;

    // internal fields
    unsigned long tdNext;
    unsigned char active;
    unsigned char pad[11];
} UhciTD;

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

#define USB_STRING_SIZE                 127

// ------------------------------------------------------------------------------------------------
// USB Speeds

#define USB_FULL_SPEED                  0x00
#define USB_LOW_SPEED                   0x01
#define USB_HIGH_SPEED                  0x02

// ------------------------------------------------------------------------------------------------
// USB Request Type

#define RT_TRANSFER_MASK                0x80
#define RT_DEV_TO_HOST                  0x80
#define RT_HOST_TO_DEV                  0x00

#define RT_TYPE_MASK                    0x60
#define RT_STANDARD                     0x00
#define RT_CLASS                        0x20
#define RT_VENDOR                       0x40

#define RT_RECIPIENT_MASK               0x1f
#define RT_DEV                          0x00
#define RT_INTF                         0x01
#define RT_ENDP                         0x02
#define RT_OTHER                        0x03

// ------------------------------------------------------------------------------------------------
// USB Device Requests

#define REQ_GET_STATUS                  0x00
#define REQ_CLEAR_FEATURE               0x01
#define REQ_SET_FEATURE                 0x03
#define REQ_SET_ADDR                    0x05
#define REQ_GET_DESC                    0x06
#define REQ_SET_DESC                    0x07
#define REQ_GET_CONF                    0x08
#define REQ_SET_CONF                    0x09
#define REQ_GET_INTF                    0x0a
#define REQ_SET_INTF                    0x0b
#define REQ_SYNC_FRAME                  0x0c

// ------------------------------------------------------------------------------------------------
// USB Hub Class Requests

#define REQ_CLEAR_TT_BUFFER             0x08
#define REQ_RESET_TT                    0x09
#define REQ_GET_TT_STATE                0x0a
#define REQ_STOP_TT                     0x0b

// ------------------------------------------------------------------------------------------------
// USB HID Interface Requests

#define REQ_GET_REPORT                  0x01
#define REQ_GET_IDLE                    0x02
#define REQ_GET_PROTOCOL                0x03
#define REQ_SET_REPORT                  0x09
#define REQ_SET_IDLE                    0x0a
#define REQ_SET_PROTOCOL                0x0b

// ------------------------------------------------------------------------------------------------
// USB Standard Feature Selectors

#define F_DEVICE_REMOTE_WAKEUP          1   // Device
#define F_ENDPOINT_HALT                 2   // Endpoint
#define F_TEST_MODE                     3   // Device

// ------------------------------------------------------------------------------------------------
// USB Hub Feature Seletcors

#define F_C_HUB_LOCAL_POWER             0   // Hub
#define F_C_HUB_OVER_CURRENT            1   // Hub
#define F_PORT_CONNECTION               0   // Port
#define F_PORT_ENABLE                   1   // Port
#define F_PORT_SUSPEND                  2   // Port
#define F_PORT_OVER_CURRENT             3   // Port
#define F_PORT_RESET                    4   // Port
#define F_PORT_POWER                    8   // Port
#define F_PORT_LOW_SPEED                9   // Port
#define F_C_PORT_CONNECTION             16  // Port
#define F_C_PORT_ENABLE                 17  // Port
#define F_C_PORT_SUSPEND                18  // Port
#define F_C_PORT_OVER_CURRENT           19  // Port
#define F_C_PORT_RESET                  20  // Port
#define F_PORT_TEST                     21  // Port
#define F_PORT_INDICATOR                22  // Port

// ------------------------------------------------------------------------------------------------
// USB Base Descriptor Types

#define USB_DESC_DEVICE                 0x01
#define USB_DESC_CONF                   0x02
#define USB_DESC_STRING                 0x03
#define USB_DESC_INTF                   0x04
#define USB_DESC_ENDP                   0x05

// ------------------------------------------------------------------------------------------------
// USB HID Descriptor Types

#define USB_DESC_HID                    0x21
#define USB_DESC_REPORT                 0x22
#define USB_DESC_PHYSICAL               0x23

// ------------------------------------------------------------------------------------------------
// USB HUB Descriptor Types

#define USB_DESC_HUB                    0x29

// ------------------------------------------------------------------------------------------------
// USB Device Descriptor

typedef struct UsbDeviceDesc
{
    unsigned char len;
    unsigned char type;
    unsigned short usbVer;
    unsigned char devClass;
    unsigned char devSubClass;
    unsigned char devProtocol;
    unsigned char maxPacketSize;
    unsigned short vendorId;
    unsigned short productId;
    unsigned short deviceVer;
    unsigned char vendorStr;
    unsigned char productStr;
    unsigned char serialStr;
    unsigned char confCount;
} __attribute__ ((PACKED)) UsbDeviceDesc;

// ------------------------------------------------------------------------------------------------
// USB Configuration Descriptor

typedef struct UsbConfDesc
{
    unsigned char len;
    unsigned char type;
    unsigned short totalLen;
    unsigned char intfCount;
    unsigned char confValue;
    unsigned char confStr;
    unsigned char attributes;
    unsigned char maxPower;
} __attribute__ ((PACKED)) UsbConfDesc;

// ------------------------------------------------------------------------------------------------
// USB String Descriptor

typedef struct UsbStringDesc
{
    unsigned char len;
    unsigned char type;
    unsigned short str[];
} __attribute__ ((PACKED)) UsbStringDesc;

// ------------------------------------------------------------------------------------------------
// USB Interface Descriptor

typedef struct UsbIntfDesc
{
    unsigned char len;
    unsigned char type;
    unsigned char intfIndex;
    unsigned char altSetting;
    unsigned char endpCount;
    unsigned char intfClass;
    unsigned char intfSubClass;
    unsigned char intfProtocol;
    unsigned char intfStr;
} __attribute__ ((PACKED)) UsbIntfDesc;

// ------------------------------------------------------------------------------------------------
// USB Endpoint Descriptor

typedef struct UsbEndpDesc
{
    unsigned char len;
    unsigned char type;
    unsigned char addr;
    unsigned char attributes;
    unsigned short maxPacketSize;
    unsigned char interval;
} __attribute__ ((PACKED)) UsbEndpDesc;

// ------------------------------------------------------------------------------------------------
// USB HID Desciptor

typedef struct UsbHidDesc
{
    unsigned char len;
    unsigned char type;
    unsigned short hidVer;
    unsigned char countryCode;
    unsigned char descCount;
    unsigned char descType;
    unsigned short descLen;
} __attribute__ ((PACKED)) UsbHidDesc;

// ------------------------------------------------------------------------------------------------
// USB Hub Descriptor

typedef struct UsbHubDesc
{
    unsigned char len;
    unsigned char type;
    unsigned char portCount;
    unsigned short chars;
    unsigned char portPowerTime;
    unsigned char current;
    // removable/power control bits vary in size
} __attribute__ ((PACKED)) UsbHubDesc;

// Hub Characteristics
#define HUB_POWER_MASK                  0x03        // Logical Power Switching Mode
#define HUB_POWER_GLOBAL                0x00
#define HUB_POWER_INDIVIDUAL            0x01
#define HUB_COMPOUND                    0x04        // Part of a Compound Device
#define HUB_CURRENT_MASK                0x18        // Over-current Protection Mode
#define HUB_TT_TTI_MASK                 0x60        // TT Think Time
#define HUB_PORT_INDICATORS             0x80        // Port Indicators

// ------------------------------------------------------------------------------------------------
// USB Device Request

typedef struct UsbDevReq
{
    unsigned char type;
    unsigned char req;
    unsigned short value;
    unsigned short index;
    unsigned short len;
} __attribute__ ((PACKED)) UsbDevReq;
// ------------------------------------------------------------------------------------------------
// USB Endpoint

typedef struct UsbEndpoint
{
    UsbEndpDesc desc;
    unsigned int toggle;
} UsbEndpoint;

// ------------------------------------------------------------------------------------------------
// USB Transfer

typedef struct UsbTransfer
{
    UsbEndpoint *endp;
    UsbDevReq *req;
    void *data;
    unsigned int len;
    unsigned char complete;
    unsigned char success;
} UsbTransfer;

// ------------------------------------------------------------------------------------------------
// USB Device

typedef struct UsbDevice
{
    struct UsbDevice *parent;
    struct UsbDevice *next;
    void *hc;
    void *drv;

    unsigned int port;
    unsigned int speed;
    unsigned int addr;
    unsigned int maxPacketSize;

    UsbEndpoint endp;

    UsbIntfDesc intfDesc;

    void (*hcControl)(struct UsbDevice *dev, UsbTransfer *t);
    void (*hcIntr)(struct UsbDevice *dev, UsbTransfer *t);

    void (*drvPoll)(struct UsbDevice *dev);
} UsbDevice;

// ------------------------------------------------------------------------------------------------
// Queue Head

typedef struct UhciQH
{
    volatile unsigned long head;
    volatile unsigned long element;

    // internal fields
    UsbTransfer *transfer;
//    Link qhLink;
    unsigned long tdHead;
    unsigned long active;
    unsigned char pad[24];
} UhciQH;

// ------------------------------------------------------------------------------------------------
// Device

typedef struct UhciController
{
    unsigned int ioAddr;
    unsigned long *frameList;
    UhciQH *qhPool;
    UhciTD *tdPool;
    UhciQH *asyncQH;
} UhciController;

#if 0
// ------------------------------------------------------------------------------------------------
static void UhciPrintTD(UhciTD *td)
{
    printf("td=0x%08x: link=0x%08x cs=0x%08x token=0x%08x buffer=0x%08x\n",
            td, td->link, td->cs, td->token, td->buffer);
}

// ------------------------------------------------------------------------------------------------
static void UhciPrintQH(UhciQH *qh)
{
    printf("qh=0x%08x: head=0x%08x element=0x%08x\n",
            qh, qh->head, qh->element);
}
#endif

// ------------------------------------------------------------------------------------------------
static UhciTD *UhciAllocTD(UhciController *hc)
{
    // TODO - better memory management
    UhciTD *end = hc->tdPool + MAX_TD;
    for (UhciTD *td = hc->tdPool; td != end; ++td)
    {
        if (!td->active)
        {
            //printf("UhciAllocTD 0x%08x\n", td);
            td->active = 1;
            return td;
        }
    }

    printf("UhciAllocTD failed\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
static UhciQH *UhciAllocQH(UhciController *hc)
{
    // TODO - better memory management
    UhciQH *end = hc->qhPool + MAX_QH;
    for (UhciQH *qh = hc->qhPool; qh != end; ++qh)
    {
        if (!qh->active)
        {
            //printf("UhciAllocQH 0x%08x\n", qh);
            qh->active = 1;
            return qh;
        }
    }

    printf("UhciAllocQH failed\n");
    return 0;
}

// ------------------------------------------------------------------------------------------------
static void UhciFreeTD(UhciTD *td)
{
    //printf("UhciFreeTD 0x%08x\n", td);
    td->active = 0;
}

// ------------------------------------------------------------------------------------------------
static void UhciFreeQH(UhciQH *qh)
{
    //printf("UhciFreeQH 0x%08x\n", qh);
    qh->active = 0;
}

// ------------------------------------------------------------------------------------------------
static void UhciInsertQH(UhciController *hc, UhciQH *qh)
{
    UhciQH *list = hc->asyncQH;
    UhciQH *end = qh;//LinkData(list->qhLink.prev, UhciQH, qhLink);

    qh->head = TD_PTR_TERMINATE;
    end->head = (unsigned long)qh | TD_PTR_QH;

//    LinkBefore(&list->qhLink, &qh->qhLink);
}

// ------------------------------------------------------------------------------------------------
static void UhciRemoveQH(UhciQH *qh)
{
//    UhciQH *prev = LinkData(qh->qhLink.prev, UhciQH, qhLink);

//    prev->head = qh->head;
//    LinkRemove(&qh->qhLink);
}

// ------------------------------------------------------------------------------------------------
static void UhciPortSet(unsigned int port, unsigned short data)
{
    unsigned int status = inportw(port);
    status |= data;
    status &= ~PORT_RWC;
    outportw(port, status);
}

// ------------------------------------------------------------------------------------------------
static void UhciPortClr(unsigned int port, unsigned short data)
{
    unsigned int status = inportw(port);
    status &= ~PORT_RWC;
    status &= ~data;
    status |= PORT_RWC & data;
    outportw(port, status);
}

// ------------------------------------------------------------------------------------------------
static void UhciInitTD(UhciTD *td, UhciTD *prev,
                       unsigned int speed, unsigned int addr, unsigned int endp, unsigned int toggle, unsigned int packetType,
                       unsigned int len, const void *data)
{
    len = (len - 1) & 0x7ff;

    if (prev)
    {
        prev->link = (unsigned long)td | TD_PTR_DEPTH;
        prev->tdNext = (unsigned long)td;
    }

    td->link = TD_PTR_TERMINATE;
    td->tdNext = 0;

    td->cs = (3 << TD_CS_ERROR_SHIFT) | TD_CS_ACTIVE;
    if (speed == USB_LOW_SPEED)
    {
        td->cs |= TD_CS_LOW_SPEED;
    }

    td->token =
        (len << TD_TOK_MAXLEN_SHIFT) |
        (toggle << TD_TOK_D_SHIFT) |
        (endp << TD_TOK_ENDP_SHIFT) |
        (addr << TD_TOK_DEVADDR_SHIFT) |
        packetType;

    td->buffer = (unsigned long)data;
}

// ------------------------------------------------------------------------------------------------
static void UhciInitQH(UhciQH *qh, UsbTransfer *t, UhciTD *td)
{
    qh->transfer = t;
    qh->tdHead = (unsigned long)td;
    qh->element = (unsigned long)td;
}

// ------------------------------------------------------------------------------------------------
static void UhciProcessQH(UhciController *hc, UhciQH *qh)
{
    UsbTransfer *t = qh->transfer;

    UhciTD *td = (UhciTD *)(unsigned long)(qh->element & ~0xf);
    if (!td)
    {
        t->success = 1;
        t->complete = 1;
    }
    else if (~td->cs & TD_CS_ACTIVE)
    {
        if (td->cs & TD_CS_NAK)
        {
            printf("NAK\n");
        }

        if (td->cs & TD_CS_STALLED)
        {
            printf("TD is stalled\n");
            t->success = 0;
            t->complete = 1;
        }

        if (td->cs & TD_CS_DATABUFFER)
        {
            printf("TD data buffer error\n");
        }
        if (td->cs & TD_CS_BABBLE)
        {
            printf("TD babble error\n");
        }
        if (td->cs & TD_CS_CRC_TIMEOUT)
        {
            printf("TD timeout error\n");
        }
        if (td->cs & TD_CS_BITSTUFF)
        {
            printf("TD bitstuff error\n");
        }
    }

    if (t->complete)
    {
        // Clear transfer from queue
        qh->transfer = 0;

        // Update endpoint toggle state
        if (t->success && t->endp)
        {
            t->endp->toggle ^= 1;
        }

        // Remove queue from schedule
        UhciRemoveQH(qh);

        // Free transfer descriptors
        UhciTD *td = (UhciTD *)(unsigned long)qh->tdHead;
        while (td)
        {
            UhciTD *next = (UhciTD *)(unsigned long)td->tdNext;
            UhciFreeTD(td);
            td = next;
        }

        // Free queue head
        UhciFreeQH(qh);
    }
}

// ------------------------------------------------------------------------------------------------
static void UhciWaitForQH(UhciController *hc, UhciQH *qh)
{
    UsbTransfer *t = qh->transfer;

    while (!t->complete)
    {
        UhciProcessQH(hc, qh);
    }
}

// ------------------------------------------------------------------------------------------------
static unsigned int UhciResetPort(UhciController *hc, unsigned int port)
{
    unsigned int reg = REG_PORT1 + port * 2;

    // Reset the port
    UhciPortSet(hc->ioAddr + reg, PORT_RESET);
resetTicks();while(1){if(getTicks()>5){break;}}//    PitWait(50);
    UhciPortClr(hc->ioAddr + reg, PORT_RESET);

    // Wait 100ms for port to enable (TODO - what is appropriate length of time?)
    unsigned int status = 0;
    for (unsigned int i = 0; i < 10; ++i)
    {
        // Delay
resetTicks();while(1){if(getTicks()>5){break;}}//        PitWait(10);

        // Get current status
        status = inportw(hc->ioAddr + reg);

        // Check if device is attached to port
        if (~status & PORT_CONNECTION)
        {
            break;
        }

        // Acknowledge change in status
        if (status & (PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE))
        {
            UhciPortClr(hc->ioAddr + reg, PORT_ENABLE_CHANGE | PORT_CONNECTION_CHANGE);
            continue;
        }

        // Check if device is enabled
        if (status & PORT_ENABLE)
        {
            break;
        }

        // Enable the port
        UhciPortSet(hc->ioAddr + reg, PORT_ENABLE);
    }

    return status;
}

// ------------------------------------------------------------------------------------------------
static void UhciDevControl(UsbDevice *dev, UsbTransfer *t)
{
    UhciController *hc = (UhciController *)dev->hc;
    UsbDevReq *req = t->req;

    // Determine transfer properties
    unsigned int speed = dev->speed;
    unsigned int addr = dev->addr;
    unsigned int endp = 0;
    unsigned int maxSize = dev->maxPacketSize;
    unsigned int type = req->type;
    unsigned int len = req->len;

    // Create queue of transfer descriptors
    UhciTD *td = UhciAllocTD(hc);
    if (!td)
    {
        return;
    }

    UhciTD *head = td;
    UhciTD *prev = 0;

    // Setup packet
    unsigned int toggle = 0;
    unsigned int packetType = TD_PACKET_SETUP;
    unsigned int packetSize = sizeof(UsbDevReq);
    UhciInitTD(td, prev, speed, addr, endp, toggle, packetType, packetSize, req);
    prev = td;

    // Data in/out packets
    packetType = type & RT_DEV_TO_HOST ? TD_PACKET_IN : TD_PACKET_OUT;

    unsigned char *it = (unsigned char *)t->data;
    unsigned char *end = it + len;
    while (it < end)
    {
        td = UhciAllocTD(hc);
        if (!td)
        {
            return;
        }

        toggle ^= 1;
        packetSize = end - it;
        if (packetSize > maxSize)
        {
            packetSize = maxSize;
        }

        UhciInitTD(td, prev, speed, addr, endp, toggle, packetType, packetSize, it);

        it += packetSize;
        prev = td;
    }

    // Status packet
    td = UhciAllocTD(hc);
    if (!td)
    {
        return;
    }

    toggle = 1;
    packetType = type & RT_DEV_TO_HOST ? TD_PACKET_OUT : TD_PACKET_IN;
    UhciInitTD(td, prev, speed, addr, endp, toggle, packetType, 0, 0);

    // Initialize queue head
    UhciQH *qh = UhciAllocQH(hc);
    UhciInitQH(qh, t, head);

    // Wait until queue has been processed
    UhciInsertQH(hc, qh);
    UhciWaitForQH(hc, qh);
}

// ------------------------------------------------------------------------------------------------
static void UhciDevIntr(UsbDevice *dev, UsbTransfer *t)
{
    UhciController *hc = (UhciController *)dev->hc;

    // Determine transfer properties
    unsigned int speed = dev->speed;
    unsigned int addr = dev->addr;
    unsigned int endp = dev->endp.desc.addr & 0xf;

    // Create queue of transfer descriptors
    UhciTD *td = UhciAllocTD(hc);
    if (!td)
    {
        t->success = 0;
        t->complete = 1;
        return;
    }

    UhciTD *head = td;
    UhciTD *prev = 0;

    // Data in/out packets
    unsigned int toggle = dev->endp.toggle;
    unsigned int packetType = TD_PACKET_IN;
    unsigned int packetSize = t->len;

    UhciInitTD(td, prev, speed, addr, endp, toggle, packetType, packetSize, t->data);

    // Initialize queue head
    UhciQH *qh = UhciAllocQH(hc);
    UhciInitQH(qh, t, head);

    // Schedule queue
    UhciInsertQH(hc, qh);
}

// ------------------------------------------------------------------------------------------------
static void UhciProbe(UhciController *hc)
{
    // Port setup
    unsigned int portCount = 2;    // TODO detect number of ports
    for (unsigned int port = 0; port < portCount; ++port)
    {
        // Reset port
        unsigned int status = UhciResetPort(hc, port);

        if (status & PORT_ENABLE)
        {
            unsigned int speed = (status & PORT_LSDA) ? USB_LOW_SPEED : USB_FULL_SPEED;
printf("WHOOOOOOOOOOOOOOOOOOOOOT PORT ENABLED!!!!\n");
//            UsbDevice *dev = UsbDevCreate();
//            if (dev)
//            {
//                dev->parent = 0;
//                dev->hc = hc;
//                dev->port = port;
//                dev->speed = speed;
//                dev->maxPacketSize = 8;
//
//                dev->hcControl = UhciDevControl;
//                dev->hcIntr = UhciDevIntr;
//
//                if (!UsbDevInit(dev))
//                {
//                    // TODO - cleanup
//                }
//            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
static void UhciControllerPoll(UsbController *controller)
{
    UhciController *hc = (UhciController *)controller->hc;

    UhciQH *qh;
    UhciQH *next;
//    ListForEachSafe(qh, next, hc->asyncQH->qhLink, qhLink)
//    {
//        if (qh->transfer)
//        {
//            UhciProcessQH(hc, qh);
//        }
//    }
}


void uhci_init(int bus,int slot,int function){


    unsigned int ioAddr = getBARaddress(bus,slot,function,0x20) & 0b11111111111111111111111111111110;

    // Controller initialization
    UhciController *hc = malloc(sizeof(UhciController));
    hc->ioAddr = ioAddr;
    hc->frameList = malloc(1024 * sizeof(unsigned long));
    hc->qhPool = (UhciQH *)malloc(sizeof(UhciQH) * MAX_QH);
    hc->tdPool = (UhciTD *)malloc(sizeof(UhciTD) * MAX_TD);

    memset(hc->qhPool, 0, sizeof(UhciQH) * MAX_QH);
    memset(hc->tdPool, 0, sizeof(UhciTD) * MAX_TD);

    // Frame list setup
    UhciQH *qh = UhciAllocQH(hc);
    qh->head = TD_PTR_TERMINATE;
    qh->element = TD_PTR_TERMINATE;
    qh->transfer = 0;
//    qh->qhLink.prev = &qh->qhLink;
//    qh->qhLink.next = &qh->qhLink;

    hc->asyncQH = qh;
    for (unsigned int i = 0; i < 1024; ++i)
    {
        hc->frameList[i] = TD_PTR_QH | (unsigned long)qh;
    }

    // Disable Legacy Support
    outportw(hc->ioAddr + REG_LEGSUP, 0x8f00);

    // Disable interrupts
    outportw(hc->ioAddr + REG_INTR, 0);

    // Assign frame list
    outportw(hc->ioAddr + REG_FRNUM, 0);
    outportl(hc->ioAddr + REG_FRBASEADD, (unsigned long)hc->frameList);
    outportw(hc->ioAddr + REG_SOFMOD, 0x40);

    // Clear status
    outportw(hc->ioAddr + REG_STS, 0xffff);

    // Enable controller
    outportw(hc->ioAddr + REG_CMD, CMD_RS);

    // Probe devices
    UhciProbe(hc);

    // Register controller
    UsbController *controller = (UsbController *)malloc(sizeof(UsbController));
//    controller->next = g_usbControllerList;
    controller->hc = hc;
//    controller->poll = UhciControllerPoll;

//    g_usbControllerList = controller;
printf("--> finished\n");
	getch();
}
