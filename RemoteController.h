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

//#define RC_DEBUG //Serial debug

class RemoteController {
public:
	RemoteController();

	/**
	 * Initialize and start the instance.
	 * Don't forget to add protocols, else most operations will not do much.
	 * @param sender_pin pin used to send the commands
	 * @param detector_pin pin used to detect to commands
	 */
	void init(uint8_t sender_pin, uint8_t detector_pin);

	void addProtocol(RemoteControlProtocolHandler *protocol);
	void removeProtocol(RemoteControlProtocolHandler *protocol);
	void getProtocols(RemoteControlProtocolHandler ***protocols, uint8_t *count);

	/**
	 * Sets the callback function that is called on detected rc-codes
	 * @param handler_fun pointer to the function
	 * @param object will be passed to the handler fun
	 */
	void setHandler(void (*handler_fun)(rc_code, void*), void *object);

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
	void (*_handler)(rc_code, void*);
	void *_handler_object;
	RemoteControlProtocolHandler **_protocols;
	byte _protocols_count;
};

#endif /* REMOTECONTROLLER_H_ */
