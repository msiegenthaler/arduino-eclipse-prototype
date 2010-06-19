#ifndef SERIALOUTPROTOCOLHANDLER_H_
#define SERIALOUTPROTOCOLHANDLER_H_

#define SERIALOUT_PROTOCOLHANDLER  // to enable

#ifdef SERIALOUT_PROTOCOLHANDLER

#include "RemoteControlProtocolHandler.h"


/**
 * Prints the received pulses to the serial port
 */
class SerialOutProtocolHandler : public RemoteControlProtocolHandler {
public:
	SerialOutProtocolHandler();
	void setHandler(void (*handler_fun)(rc_code));
	void processPulse(uint16_t pulseHighUs, uint16_t pulseLowUs);
	PulseIterator* makePulseIterator(rc_code code);
};

#endif
#endif /* SERIALOUTPROTOCOLHANDLER_H_ */
