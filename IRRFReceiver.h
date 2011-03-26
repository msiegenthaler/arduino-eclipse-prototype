#ifndef IRRFRECEIVER_H_
#define IRRFRECEIVER_H_

#include "../gidaivel-arduino-base/Avieul.h"
#include "../gidaivel-arduino-base/SubscriptionManager.h"
#include "RemoteController.h"
#include "RemoteControlProtocolHandler.h"

#define DEBUG_IRRF

/**
 * Infrared or RF receiver.
 */
class IRRFReceiver  : public AvieulService {
public:
	IRRFReceiver(RemoteController *rc);

	void handleReceivedCode(rc_code code);

protected:
	virtual void processCall(uint16_t callType, XBeeAddress from, uint8_t* payload, uint8_t payload_length);
	virtual bool processRequest(uint16_t requestType, XBeeAddress from, uint8_t* payload, uint8_t payload_length);
	virtual bool addSubscription(XBeeAddress from, uint16_t subscriptionType);
	virtual void removeSubscription(XBeeAddress from, uint16_t subscriptionType);

	inline bool addSingleFixedProtocol(uint8_t id, uint8_t *data, uint8_t len);
	inline void removeProtocol(uint8_t id);
	inline void removeAllProtocols();

private:
	RemoteController *_rc;
	SubscriptionManager *_subscription;
};

#endif /* IRRFRECEIVER_H_ */
