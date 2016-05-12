#ifndef serial_H_
#define serial_H_

#include <usbd/device.h>
#include <types.h>

struct SerialDevice {
	/** Standard driver data header. */
	//struct UsbDriverDataHeader Header;
	/** Serial out endpoint (Bulk endpointer) */
	u32 OutEndpoint;
    /** Serial in endpoint (Bulk endpointer) */
    u32 InEndpoint;
    
};

Result SerialAttach(struct UsbDevice *device, u32 interface);
int SendToUSB(char *buf, int bufSize);
int RecvFromUSB(const char *buf, int bufSize);
#endif