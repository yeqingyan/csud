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
	LOG("SERIAL attach done!\n");
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
	LOG("RecvFromUSB Called\n");
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
	//LOGF("TOS Receive Result = %x\n", result);
	//char  *ptr = buf;
	//ptr += 2;	// Skip 2 serial break bytes from QEMU
	//LOGF("Reply %s\n",ptr);
	return result;
}