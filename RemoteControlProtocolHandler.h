#ifndef RemoteControlProtocolHandler_h
#define RemoteControlProtocolHandler_h

#include "RemoteController.h"

/**
 * Interface for pulse iterators.
 */
class PulseIterator {
public:
	/**
	 * Returns the next pulse.
	 * @param pulseHighUs duration in microseconds of the HIGH-pulse
	 * @param pulseLowUs duration in microseconds of the LOW-pulse
	 * @return true if there are more pulses
	 */
	virtual bool nextPulse(uint16_t *pulseHighUs, uint16_t *pulseLowUs);
};

/**
 * Interface for a remote control protocol.
 */
class RemoteControlProtocolHandler {
public:
	/**
	 * Sets the callback function that is called on detected rc-codes
	 * @param handler_fun pointer to the function
	 * @param object will be passed to the handler_fun
	 */
	virtual void setHandler(void (*handler_fun)(rc_code, void*), void *object);

	/**
	 * Process the pulses and perform the code detection on it.
	 *
	 * @param pulseHighUs duration in microseconds of the HIGH-pulse
	 * @param pulseLowUs duration in microseconds of the LOW-pulse
	 */
	virtual void processPulse(uint16_t pulseHighUs, uint16_t pulseLowUs);

	/**
	 * Make the pulses for RC-codes.
	 *
	 * @param code the rc code the generate the pulses for.
	 * @return the pulse iterator or nil if unknown code
	 */
	virtual PulseIterator* makePulseIterator(rc_code code);
};

/**
 * Remote control protocol handler that emits a single type only.
 */
class SingleTypeRemoteControlProtocolHandler : public RemoteControlProtocolHandler {
public:
	virtual uint8_t getType();
};

#endif
