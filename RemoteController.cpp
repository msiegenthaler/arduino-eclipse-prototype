#include "RemoteController.h"
#include "RemoteControlProtocolHandler.h"

//ISR
uint8_t  rcInPin;
volatile bool lastPinState = false;
volatile uint32_t lastStateChange = 0;
volatile uint8_t lastHighDuration = 0;
volatile uint16_t duration[RC_BUFFER_SIZE]; //High-Low cycle (highDuration << 8 | lowDuration)
volatile uint8_t duration_pos = 0;
void rc_isr() {
	bool s = digitalRead(2) == HIGH;
	if (s != lastPinState) {
		uint32_t t = micros();
		uint16_t d;
		if (t >= lastStateChange) d = t - lastStateChange;
		else d = 0xFFFFFFFF - lastStateChange + t;

		if (!s) {
			//Switched to low
			lastHighDuration = (d + RC_PULSE_RESOLUTION_HALF) / RC_PULSE_RESOLUTION;
		} else {
			//Switched to high, cycle complete
			if (duration_pos < RC_BUFFER_SIZE) {
				uint8_t dlow = (d + RC_PULSE_RESOLUTION_HALF) / RC_PULSE_RESOLUTION;
				duration[duration_pos] = (((uint16_t)lastHighDuration) << 8) | dlow;
				duration_pos++;
			} // else buffer is full, ignore pulse
		}
		lastStateChange = t;
		lastPinState = s;
	}
}



RemoteController::RemoteController() {
	_handler = 0;
	_protocols_count = 0;
}

void RemoteController::init(uint8_t sender_pin, uint8_t detector_pin) {
	int interrupt;
	if (detector_pin == 2) interrupt = 0;
	else if (detector_pin == 3) interrupt = 1;
	else if (detector_pin == 21) interrupt = 2;
	else if (detector_pin == 22) interrupt = 3;
	else if (detector_pin == 19) interrupt = 4;
	else if (detector_pin == 18) interrupt = 5;
	else return; //failed
	rcInPin = detector_pin;
	pinMode(detector_pin, INPUT);
	attachInterrupt(interrupt, rc_isr, CHANGE);

	_sender_pin = sender_pin;
	pinMode(_sender_pin, OUTPUT);

	_protocols_count = 0;
	_protocols = NULL;

#ifdef RC_DEBUG
	Serial.print("rc: initialized.");
#endif
}


inline bool exists(RemoteControlProtocolHandler **all, uint8_t count, RemoteControlProtocolHandler *toFind) {
	for (uint8_t i=0; i<count; i++) {
		if (all[i] == toFind) return true;
	}
	return false;
}

void RemoteController::addProtocol(RemoteControlProtocolHandler *protocol) {
	if (exists(_protocols, _protocols_count, protocol)) return; //already exists

	protocol->setHandler(_handler);

	size_t size = sizeof(RemoteControlProtocolHandler*) * (_protocols_count + 1);
	RemoteControlProtocolHandler **na = (RemoteControlProtocolHandler**)malloc(size);
	memcpy(na, _protocols, size);
	na[_protocols_count] = protocol;
	free(_protocols);
	_protocols = na;
	_protocols_count++;

#ifdef RC_DEBUG
	Serial.println("rc: added protocol");
#endif
}
void RemoteController::removeProtocol(RemoteControlProtocolHandler *protocol) {
	if (!exists(_protocols, _protocols_count, protocol)) return; //does not exist

	size_t size = sizeof(RemoteControlProtocolHandler*) * (_protocols_count - 1);
	RemoteControlProtocolHandler **na = (RemoteControlProtocolHandler**)malloc(size);
	uint8_t c = 0;
	for (uint8_t i=0; i<_protocols_count; i++) {
		if (_protocols[i] == protocol) break;
		na[c++] = _protocols[i];
	}
	free(_protocols);
	_protocols = na;
	_protocols_count = c;

	protocol->setHandler(NULL);

#ifdef RC_DEBUG
	Serial.println("rc: removed protocol");
#endif
}


void RemoteController::setHandler(void (*handler_fun)(rc_code)) {
	_handler = handler_fun;

	for (int i=0; i<_protocols_count; i++) {
		_protocols[i]->setHandler(_handler);
	}

#ifdef RC_DEBUG
	Serial.println("rc: set handler");
#endif
}

void RemoteController::detect() {
	//copy buffer (with disabled interrupts)
	uint8_t len;
	uint16_t *buffer = 0;
	noInterrupts();
	len = duration_pos;
	if (len > 0) {
		uint16_t size = sizeof(uint16_t)*len;
		buffer = (uint16_t*) malloc(size);
		memcpy(buffer, (void*)&duration[0], size);
		duration_pos = 0;
	} else len = 0;
	interrupts();
	//enabled interrupts

	if (len > 0) {
		for (uint8_t i=0; i<len; i++) {
			uint16_t highDuration = (buffer[i] >> 8) * RC_PULSE_RESOLUTION;
			uint16_t lowDuration = (buffer[i] & 0xFF) * RC_PULSE_RESOLUTION;

			for (uint8_t j=0; j<_protocols_count; j++)
				_protocols[j]->processPulse(highDuration, lowDuration);
		}

		free(buffer);
	}
#ifdef RC_DEBUG
	if (len == RC_BUFFER_SIZE) {
		Serial.println("rc: buffer overflow");
	}
#endif
}

void RemoteController::send_code(rc_code code) {
	//TODO
}
