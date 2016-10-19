#include <device/hub.h>
#include <device/serial.h>
#include <hcd/hcd.h>
#include <platform/platform.h>
#include <usbd/device.h>
#include <usbd/devicerequest.h>
#include <usbd/descriptors.h>
#include <usbd/pipe.h>
#include <usbd/usbd.h>

int bulk_out_endpoint = 0;
int bulk_in_endpoint = 0;
int serial_interface = 0;
struct UsbDevice* serialDevice = NULL;

#define SET_LINE_REQUEST_TYPE       0x21
#define SET_LINE_REQUEST            0x20
#define VENDOR_WRITE_REQUEST_TYPE   0x40
#define VENDOR_WRITE_REQUEST        0x01
#define VENDOR_READ_REQUEST_TYPE    0xc0
#define VENDOR_READ_REQUEST         0x01

#define SerialMessageTimeout 100

void SerialLoad() {
    LOG("CSUD: TOS Serial driver version 0.1\n"); 
    InterfaceClassAttach[InterfaceClassReserved] = SerialAttach;    
}

Result SerialAttach(struct UsbDevice *device, u32 interface) {
    int endpoint_count = device->Interfaces[interface].EndpointCount;
    serial_interface = interface;
    // LOGF("Found %d endpoints.\n", endpoint_count);
    int i = 0;
    for (i= 0; i <=endpoint_count; i++) {
        struct UsbEndpointDescriptor ep_temp = device->Endpoints[interface][i];
        if (ep_temp.Attributes.Type == Bulk) {
            if (ep_temp.EndpointAddress.Direction == In) {
                bulk_in_endpoint = ep_temp.EndpointAddress.Number;	
            } else if (ep_temp.EndpointAddress.Direction == Out) {
                bulk_out_endpoint = ep_temp.EndpointAddress.Number;
            } else {
                LOG("WARNING: Meet strange bulk endpoint direction!\n");
            }
        }
    }
    LOGF("Found bulk in endpoint %d, bulk out endpoints %d.\n", bulk_in_endpoint, bulk_out_endpoint);
    serialDevice = device;
    
    // From Linux kernel drivers/usb/serial/pl2303.c 
    // Reverse engineering code, no document, hope I never need this. 
     char vender_buf[1];
     pl2303_vendor_read(serialDevice, 0x8484, vender_buf);
	 pl2303_vendor_write(serialDevice, 0x0404, 0);
	 pl2303_vendor_read(serialDevice, 0x8484, vender_buf);
	 pl2303_vendor_read(serialDevice, 0x8383, vender_buf);
	 pl2303_vendor_read(serialDevice, 0x8484, vender_buf);
	 pl2303_vendor_write(serialDevice, 0x0404, 1);
	 pl2303_vendor_read(serialDevice, 0x8484, vender_buf);
	 pl2303_vendor_read(serialDevice, 0x8383, vender_buf);
	 pl2303_vendor_write(serialDevice, 0, 1);
	 pl2303_vendor_write(serialDevice, 1, 0);
    
    // Initialise pl2303 
    // From Linux kernel drivers/usb/serial/pl2303.c  
    
    //LOG("SERIAL attach done!\n");
    //return OK;





    char buf[7];
    buf[0] = 0x30;
    buf[1] = 0x0;
    buf[2] = 0x0;
    buf[3] = 0x0;	// buf[0]:buf[3] baud rate value
    buf[4] = 2;		// line control 2 stop bits
    buf[5] = 0;		// No parity
    buf[6] = 8;		// word length 8 bits
    Result result;
    if ((result = UsbControlMessage(
        device, 
        (struct UsbPipeAddress) { 
            .Type = Control, 
            .Speed = device->Speed, 
            .EndPoint = 0 , 
            .Device = device->Number, 
            .Direction = Out,
            .MaxSize = SizeFromNumber(device->Descriptor.MaxPacketSize0),
        },
        &buf,
        7,
        &(struct UsbDeviceRequest) {
            .Request = SET_LINE_REQUEST,
            .Type = SET_LINE_REQUEST_TYPE,
            .Index = 0,
            .Value = 0,
            .Length = 0,
        },
        SerialMessageTimeout)) != OK) {
        return result;
    }
    
    LOG("SERIAL attach done!!!!\n");
    return OK;
}

int SendToUSB(char *buf, int bufSize) {
    Result result;
    struct UsbDevice *device = serialDevice;
    result = HcdSumbitBulkMessage(
        device, 		
        (struct UsbPipeAddress) { 
            .Type = Bulk, 
            .Speed = device->Speed, 
            .EndPoint = bulk_out_endpoint, 
            .Device = device->Number, 
            .Direction = Out,
            .MaxSize = SizeFromNumber(device->Endpoints[serial_interface][bulk_in_endpoint].Packet.MaxSize),
        },
        buf,
        bufSize);
    //LOGF("TOS Send Result = %x\n", result);
    return result;
}

int RecvFromUSB(const char *buf, int bufSize) {
    Result result;
    struct UsbDevice *device = serialDevice;
    //LOG("RecvFromUSB Called\n");
    result = HcdSumbitBulkMessage(
        device, 		
        (struct UsbPipeAddress) { 
            .Type = Bulk, 
            .Speed = device->Speed, 
            .EndPoint = bulk_in_endpoint, 
            .Device = device->Number, 
            .Direction = In, 
            .MaxSize = SizeFromNumber(device->Endpoints[serial_interface][bulk_in_endpoint].Packet.MaxSize),
        },
        (void *)buf,
        bufSize);
    LOGF("TOS Receive Result = %x\n", result);
    //char  *ptr = buf;
    //ptr += 2;	// Skip 2 serial break bytes from QEMU
    //LOGF("Reply %s\n",ptr);
    return result;
}

int pl2303_vendor_read(struct UsbDevice *device, unsigned int value, unsigned char buf[1])
{
	Result result;
    
    result = UsbControlMessage(
        device, 
        (struct UsbPipeAddress) { 
            .Type = Control, 
            .Speed = device->Speed, 
            .EndPoint = 0 , 
            .Device = device->Number, 
            .Direction = In,
            .MaxSize = SizeFromNumber(device->Descriptor.MaxPacketSize0),
        },
        &buf,
        1,
        &(struct UsbDeviceRequest) {
            .Request = VENDOR_READ_REQUEST,
            .Type = VENDOR_READ_REQUEST_TYPE,
            .Index = 0,
            .Value = value,
            .Length = 1,
        },
        SerialMessageTimeout);
        
    if (result != OK) {
        LOG("TOS pl2303 vender read failed.\n");
    }            
	return 0;
}

int pl2303_vendor_write(struct UsbDevice *device, unsigned int value, unsigned int index)
{
	Result result;
    
    result = UsbControlMessage(
        device, 
        (struct UsbPipeAddress) { 
            .Type = Control, 
            .Speed = device->Speed, 
            .EndPoint = 0 , 
            .Device = device->Number, 
            .Direction = Out,
            .MaxSize = SizeFromNumber(device->Descriptor.MaxPacketSize0),
        },
        NULL,
        0,
        &(struct UsbDeviceRequest) {
            .Request = VENDOR_WRITE_REQUEST,
            .Type = VENDOR_WRITE_REQUEST_TYPE,
            .Index = index,
            .Value = value,
            .Length = 0,
        },
        SerialMessageTimeout);
        
    if (result != OK) {
        LOG("TOS pl2303 vender write failed.\n");   
    }            
	return 0;
}

