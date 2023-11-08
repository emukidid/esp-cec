#include "CEC_Device.h"

// NodeMCU ESP8266 Pins   bootstrap
// D0  GPIO16  DeepSleep  blue LED connected to 3V3
// D1  GPIO5
// D2  GPIO4
// D3  GPIO0              pull-up (SPI boot) / pull-down (UART boot)
// D4  GPIO2   TxD1       pull-up, blue LED on ESP12E module
// D5  GPIO14
// D6  GPIO12
// D7  GPIO13
// D8  GPIO15             pull-down
// D9  GPIO3   RxD0
// D10 GPIO1   TxD0
// CLK GPIO6   SD_CLK
// SD0 GPIO7   SD_D0
// SD1 GPIO8   SD_D1
// SD2 GPIO9   SD_D2
// SD3 GPIO10  SD_D3
// CMD GPIO11  SD_CMD

static boolean noResponseYet = true;
static boolean weReady = false;
static boolean startupSent = false;

#define CEC_GPIO 5
#define CEC_DEVICE_TYPE CEC_Device::CDT_PLAYBACK_DEVICE
#define CEC_PHYSICAL_ADDRESS 0x3000

// implement application specific CEC device
class MyCEC_Device : public CEC_Device
{
protected:
	virtual bool LineState();
	virtual void SetLineState(bool);
	virtual void OnReady(int logicalAddress);
	virtual void OnReceiveComplete(unsigned char* buffer, int count, bool ack);
	virtual void OnTransmitComplete(unsigned char* buffer, int count, bool ack);
};

bool MyCEC_Device::LineState()
{
	int state = digitalRead(CEC_GPIO);
	return state != LOW;
}

void MyCEC_Device::SetLineState(bool state)
{
	if (state) {
		pinMode(CEC_GPIO, INPUT_PULLUP);
	} else {
		digitalWrite(CEC_GPIO, LOW);
		pinMode(CEC_GPIO, OUTPUT);
	}
	// give enough time for the line to settle before sampling it
	delayMicroseconds(50);
}

void MyCEC_Device::OnReady(int logicalAddress)
{
  // At this point we have a logical address
  weReady = true;
	// This is called after the logical address has been allocated

	unsigned char buf[4] = {0x84, CEC_PHYSICAL_ADDRESS >> 8, CEC_PHYSICAL_ADDRESS & 0xff, CEC_DEVICE_TYPE};

	DbgPrint("Device ready, Logical address assigned: %d\n", logicalAddress);

	TransmitFrame(0xf, buf, 4); // <Report Physical Address>
}

void MyCEC_Device::OnReceiveComplete(unsigned char* buffer, int count, bool ack)
{
	// This is called when a frame is received.  To transmit
	// a frame call TransmitFrame.  To receive all frames, even
	// those not addressed to this device, set Promiscuous to true.
	DbgPrint("Packet received at %ld: %02X", millis(), buffer[0]);
	for (int i = 1; i < count; i++)
		DbgPrint(":%02X", buffer[i]);
	if (!ack)
		DbgPrint(" NAK");
	DbgPrint("\n");

	// Ignore messages not sent to us
	if ((buffer[0] & 0xf) != LogicalAddress())
		return;

	// No command received?
	if (count < 1)
		return;

	switch (buffer[1]) {
	case 0x83: { // <Give Physical Address>
		unsigned char buf[4] = {0x84, CEC_PHYSICAL_ADDRESS >> 8, CEC_PHYSICAL_ADDRESS & 0xff, CEC_DEVICE_TYPE};
		TransmitFrame(0xf, buf, 4); // <Report Physical Address>
		break;
	}
	case 0x8c: // <Give Device Vendor ID>
		TransmitFrame(0xf, (unsigned char*)"\x87\x00\xF1\x0E", 4); // <Device Vendor ID>
		break;
	}
}

void MyCEC_Device::OnTransmitComplete(unsigned char* buffer, int count, bool ack)
{
	// This is called after a frame is transmitted.
	DbgPrint("Packet sent at %ld: %02X", millis(), buffer[0]);
	for (int i = 1; i < count; i++)
		DbgPrint(":%02X", buffer[i]);
	if (!ack)
		DbgPrint(" NAK");
	DbgPrint("\n");

  // If we haven't succeeded in sending anything, assume the TV is off
  if(!ack) {
    noResponseYet = true;
    startupSent = false;
  }
  // If we've sent something but haven't had a response yet, the TV hasn't turned on yet
  if(ack && noResponseYet) {
    startupSent = false;
  }
}


MyCEC_Device device;

void setup()
{
  weReady = false;
  noResponseYet = true;
  startupSent = false;
	pinMode(CEC_GPIO, INPUT_PULLUP);

	Serial.begin(115200);
  DbgPrint("Serial is up\n");
	device.Initialize(CEC_PHYSICAL_ADDRESS, CEC_DEVICE_TYPE, true); // Promiscuous mode
  DbgPrint("Init complete.\n");
}

void loop()
{
  if(weReady && noResponseYet && !startupSent) {
    delay(500);
    DbgPrint("Sending out startup\n");
    unsigned char buffer[4] = {0, 0, 0, 0};
    buffer[0] = 0x04;
    device.TransmitFrame(0, buffer, 1);
    startupSent = true;
  }
	device.Run();
}
