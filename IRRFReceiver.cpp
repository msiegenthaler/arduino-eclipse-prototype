#include "IRRFReceiver.h"

IRRFReceiver::IRRFReceiver(RemoteController *rc) {
	_rc = rc;
//	_rc->setHandler(handler); //TODO
}

bool IRRFReceiver::processRequest(uint16_t requestType, XBeeAddress from, uint8_t* payload, uint8_t payload_length) {
	return false;
}

bool IRRFReceiver::addSubscription(XBeeAddress from, uint16_t subscriptionType) {
	switch (subscriptionType) {
	case 0001: // detected ir commands
		addSubscriber(from);
		return true;
	default:
		return false;
	}
}


void IRRFReceiver::removeSubscription(XBeeAddress from, uint16_t subscriptionType) {
	switch (subscriptionType) {
	case 0001: // detected ir commands
		removeSubscriber(from);
		break;
	default: break;
	}
}

inline bool exists(XBeeAddress *all, uint8_t count, XBeeAddress toFind) {
	for (uint8_t i=0; i<count; i++) {
		if (all[i] == toFind) return true;
	}
	return false;
}

void IRRFReceiver::addSubscriber(XBeeAddress from) {
	if (exists(_subscribers, _subscriber_count, from)) return; //already subscribed

	size_t size = sizeof(XBeeAddress) * (_subscriber_count + 1);
	XBeeAddress* na = (XBeeAddress*)malloc(size);
	memcpy(na, _subscribers, size);
	na[_subscriber_count] = from;
	free(_subscribers);
	_subscribers = na;
	_subscriber_count++;
}

void IRRFReceiver::removeSubscriber(XBeeAddress from) {
	if (!exists(_subscribers, _subscriber_count, from)) return; //not subscribed

	size_t size = sizeof(XBeeAddress) * (_subscriber_count - 1);
	XBeeAddress* na = (XBeeAddress*)malloc(size);
	uint8_t c=0;
	for (uint8_t i=0; i<_subscriber_count; i++) {
		if (_subscribers[i] == from) break;
		na[c++] = _subscribers[i];
	}
	free(_subscribers);
	_subscribers = na;
	_subscriber_count = c;
}

