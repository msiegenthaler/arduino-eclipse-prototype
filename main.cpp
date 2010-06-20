#include "WProgram.h"
#include "RemoteController.h"
#include "RemoteControlProtocolHandler.h"
#include "SerialOutProtocolHandler.h"
#include "ComposedProtocolHandler.h"
#include "../arduino-xbee/Series1XBee.h"
#include "../arduino-xbee/LowlevelXBee.h"
#include "../arduino-xbee/NewSoftSerialApiModeXBee.h"
#include "IRRFReceiver.h"


extern "C" void __cxa_pure_virtual(void);
void __cxa_pure_virtual(void) {}

void * operator new(size_t size);
void operator delete(void * ptr);
void * operator new(size_t size) {
   return malloc(size);
}
void operator delete(void * ptr) {
	free(ptr);
}


// variables
int ledPin = 13; // LED connected to digital pin 13


RemoteController rc;

/*
RemoteControlProtocolHandler* makeSony() {
	uint16_t preSeq[] =  {0, 2450};
	uint16_t zeroSeq[] = {500,  650};
	uint16_t oneSeq[]  = {500, 1250};
	return new ComposedProtocolHandler(0x1,
			new FixedPartHandler(preSeq, 2),
			new SingleBitPartHandler(zeroSeq, oneSeq, 2, 12),
			NULL, 3);
}

RemoteControlProtocolHandler* makeAccuphase() {
	uint16_t preSeq[] =  {    0, 8500, 4250, 600};
	uint16_t zeroSeq[] = {  500,  600 };
	uint16_t oneSeq[]  = { 1550,  600 };
	return new ComposedProtocolHandler(0x2,
			new FixedPartHandler(preSeq, 4),
			new SingleBitPartHandler(zeroSeq, oneSeq, 2, 32),
			NULL, 1);
}

RemoteControlProtocolHandler* makeIntertechno() {
	/////////////////////////////////
	//            on      off
	//G1 D1 => 0x100015 0x100014
	//G1 D2 => 0x104015	0x104014
	//G1 D3 => 0x101015	0x101014
	//G1 D4 => 0x105015	0x105014
	//G2 D1 => 0x100415	0x100414
	//G2 D2 => 0x104415	0x104414
	//G2 D3 => 0x101415	0x101414
	//G2 D4 => 0x105455	0x105414
	//G3 D1 => 0x100115	0x100114
	//.............
	//Keyup => 0x10F0D5
	/////////////////////////////////
	uint16_t preSeq[] =  {  400, 12000, 400, 1100, 400, 1100 };
	uint16_t zeroSeq[] = {  400, 1100 };
	uint16_t oneSeq[]  = { 1100,  400 };
	return new ComposedProtocolHandler(0x3,
			new FixedPartHandler(preSeq, 6),
			new SingleBitPartHandler(zeroSeq, oneSeq, 2, 22),
			NULL, 2);
}

RemoteControlProtocolHandler* makeTelis4() {
//	uint16_t preSeq[] =  { 10000, 600, 7900, 2250, 4850, 1200 };
//	uint16_t preSeq[] =  { 10000, 600, 2500, 2500, 2500, 2500, 4850, 1200 };
	uint16_t preSeq[] =  { 2500, 2500, 2500, 2500, 4850, 1200 };
	uint16_t zeroSeq[] = {  600 };
	uint16_t oneSeq[]  = { 1200 };
	return new ComposedProtocolHandler(0x4,
			new FixedPartHandler(preSeq, 6),
			new SingleBitPartHandler(zeroSeq, oneSeq, 1, 32),
			NULL, 1);
}

void rc_handler(rc_code code, void* object) {
	Serial.print("Detected code ");
	Serial.print(code.type, 10);
	Serial.print(" - 0x");
	Serial.println(code.code, HEX);
//	Serial.print(" - ");
//	Serial.println(code.code, 2);
}
*/

Avieul *avieul;

void setup() {
	Serial.begin(38400); // for debugging
	pinMode(ledPin, OUTPUT); // sets the digital pin as output
	digitalWrite(ledPin, LOW);

	NewSoftSerial *serial = new NewSoftSerial(4, 5);
	serial->begin(19200);
	LowlevelXBee *lowlevel = new NewSoftSerialApiModeXBee(serial);
	Series1XBee *xbee = new Series1XBee(lowlevel);

	rc.init(3, 2);
	IRRFReceiver *irrf = new IRRFReceiver(&rc);

	AvieulService* services[] = { irrf };
	avieul = new Avieul(xbee, services, 1);

	Serial.println("Started");
}

void loop() {
	avieul->process();
	rc.detect();
}



int main(void) {
	init();
	setup();
	for (;;) loop();
	return 0;
}
