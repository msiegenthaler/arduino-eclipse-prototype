#ifndef REMOTECONTROLLER_H_
#define REMOTECONTROLLER_H_

#undef int
#undef abs
#undef double
#undef float
#undef round
#include "WProgram.h"

typedef struct _rc_code_t {
	uint8_t type;
	uint32_t code;
} rc_code;

class RemoteControlProtocolHandler;

#define RC_BUFFER_SIZE 255
#define RC_PULSE_RESOLUTION 50 //in us
#define RC_PULSE_RESOLUTION_HALF 25 //in us

#define RC_DEBUG //Serial debug

class RemoteController {
public:
	RemoteController();

	/**
	 * Initialize and start the instance.
	 * @param sender_pin pin used to send the commands
	 * @param detector_pin pin used to detect to commands
	 * @param protocols protocol implementations
	 * @param protocol_count Length of protocols
	 */
	void init(uint8_t sender_pin, uint8_t detector_pin, RemoteControlProtocolHandler **protocols, uint8_t protocols_count);

	/**
	 * Sets the callback function that is called on detected rc-codes
	 * @param handler_fun pointer to the function
	 */
	void setHandler(void (*handler_fun)(rc_code));

	/**
	 * Detects RC-codes. Call in every loop pass.
	 */
	void detect();

	/**
	 * Send the RC-code.
	 */
	void send_code(rc_code code);

private:
	uint8_t _sender_pin;
	void (*_handler)(rc_code);
	RemoteControlProtocolHandler **_protocols;
	byte _protocols_count;
};

#endif /* REMOTECONTROLLER_H_ */
