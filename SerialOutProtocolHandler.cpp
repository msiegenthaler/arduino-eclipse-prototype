#include "SerialOutProtocolHandler.h"
#ifdef SERIALOUT_PROTOCOLHANDLER

SerialOutProtocolHandler::SerialOutProtocolHandler() {
}

void SerialOutProtocolHandler::setHandler(void (*handler_fun)(rc_code,void*), void *object) {
	//ignore
}

void printFixedWidth(uint16_t data, uint8_t len) {
	uint32_t f = 1;
	uint8_t l = 0;
	while (data / f > 9) {
		l++;
		f = f * 10;
	}
	for (uint8_t i=l; i<len; i++) Serial.print(" ");
	Serial.print(data, 10);
}

void SerialOutProtocolHandler::processPulse(uint16_t pulseHighUs, uint16_t pulseLowUs) {
	//Serial.print("Pulse ");
	printFixedWidth(pulseHighUs, 5);
	Serial.print("/");
	//Serial.print("us high, ");
	printFixedWidth(pulseLowUs, 5);
	//Serial.println("us low");
	Serial.println();
}

PulseIterator* SerialOutProtocolHandler::makePulseIterator(rc_code code) {
	return NULL;
}


#endif
