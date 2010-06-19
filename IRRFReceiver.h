#ifndef IRRFRECEIVER_H_
#define IRRFRECEIVER_H_

#include "../gidaivel-arduino-base/Avieul.h"
#include "RemoteController.h"


/**
 * Infrared or RF receiver.
 */
class IRRFReceiver  : public AvieulService {
public:
	IRRFReceiver(RemoteController *rc);

protected:
	virtual bool processRequest(uint16_t requestType, XBeeAddress from, uint8_t* payload, uint8_t payload_length);
	virtual bool addSubscription(XBeeAddress from, uint16_t subscriptionType);
	virtual void removeSubscription(XBeeAddress from, uint16_t subscriptionType);
	inline void addSubscriber(XBeeAddress from);
	inline void removeSubscriber(XBeeAddress from);

private:
	RemoteController *_rc;
	XBeeAddress *_subscribers;
	uint8_t _subscriber_count;
};

#endif /* IRRFRECEIVER_H_ */
