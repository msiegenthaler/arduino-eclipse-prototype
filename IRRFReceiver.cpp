#include "IRRFReceiver.h"
#include "ComposedProtocolHandler.h"

void handle_rcrf(rc_code code, void *object) {
	IRRFReceiver *receiver = (IRRFReceiver*)object;
	receiver->handleReceivedCode(code);
}

IRRFReceiver::IRRFReceiver(RemoteController *rc) {
	_rc = rc;
	_rc->setHandler(handle_rcrf, this);
}

bool IRRFReceiver::processRequest(uint16_t requestType, XBeeAddress from, uint8_t* payload, uint8_t payload_length) {
	switch (requestType) {
	case 0x0001: {// load single-bit fixed-length
		if (payload_length < 9) return false;
		if (addSingleFixedProtocol(payload[0], &payload[1], payload_length-1)) {
			//ok
			uint8_t data[] = { 0, 0, 0, 0, payload[0], 0x01 };
			fillResponseHeader(data, requestType);
			_sender->send(from, data, 5);
		} else {
			//failed
			uint8_t data[] = { 0, 0, 0, 0, payload[0], 0xFF };
			fillResponseHeader(data, requestType);
			_sender->send(from, data, 5);
		}
		return true;
	}
	case 0x0002: { // unload
		if (payload_length < 1) return false;
		removeProtocol(payload[0]);
		uint8_t data[] = { 0, 0, 0, 0, payload[0] };
		fillResponseHeader(data, requestType);
		_sender->send(from, data, 5);
		return true;
	}
	case 0x0003: { // unload all
		removeAllProtocols();
		uint8_t data[] = { 0, 0, 0, 0 };
		fillResponseHeader(data, requestType);
		_sender->send(from, data, 4);
		return true;
	}
	default:
		return false;
	}
}

bool IRRFReceiver::addSubscription(XBeeAddress from, uint16_t subscriptionType) {
	switch (subscriptionType) {
	case 0x0001: // detected ir commands
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

void convertUnit(uint16_t unitInUs, uint8_t count, uint8_t *data, uint16_t *out) {
	for (uint8_t i=0; i<count; i++) {
		out[i] = unitInUs * data[i];
	}
}

// 0	unit in us (2 bytes): units used in the rest of the message in microseconds (0-65535)
// 2	repeats (1 byte): number of time the request is repeated
// 3	preample length (1 byte): number of pulses in the preample
// 4	bit length (1 byte): number of pulses for a bit
// 5	suffix length (1 byte): number of pulses in the suffix. (preample + 2*bit + suffix <= 89)
// 6 	bit count (1 byte): Number of bits in a command (1-94)
// 7	preample pulse (1 byte)*: [ exactly preample length times ]
//		pulses in units that make up a "zero" bit (0-25%). The "detection" starts with a low state.
// *	bit "zero" pulse (1 byte)*: [ exactly bit length times ]
//		pulses in units that make up a "zero" bit
// *	bit "one" pulse (1 byte)*: [ exactly bit length times ]
//		pulses in units that make up a "one" bit
// *	suffix pulse (1 byte)*: [ exactly suffix length times ]
//		pulses in units that make up the suffix
bool IRRFReceiver::addSingleFixedProtocol(uint8_t id, uint8_t *data, uint8_t len) {
	uint16_t unitInUs = ((uint16_t)data[0]) << 8 | data[1];
	uint8_t repeats = data[2];
	uint8_t pre_len = data[3];
	uint8_t bit_len = data[4];
	uint8_t suffix_len = data[5];
	uint8_t bit_count = data[6];
	if (pre_len+bit_len+suffix_len+7 > len) {
#ifdef DEBUG_IRRF
		Serial.print("irrfr: addsfp: malformed (count)");
#endif
		return false;
	}
	if (bit_len<1) {
#ifdef DEBUG_IRRF
		Serial.print("irrfr: addsfp: malformed (bit len)");
#endif
		return false;
	}
	uint8_t *pre_pulses = &data[7];
	uint8_t *bit0_pulses = &data[7+pre_len];
	uint8_t *bit1_pulses = &data[7+pre_len+bit_len];
	uint8_t *suffix_pulses = &data[7+pre_len+bit_len*2];


	PartHandler *pre;
	if (pre_pulses > 0) {
		uint16_t us[pre_len];
		convertUnit(unitInUs, pre_len, pre_pulses, us);
		pre = new FixedPartHandler(us, pre_len);
	} else
		pre = NULL;

	DataPartHandler *bit;
	uint16_t us_0[bit_len];
	uint16_t us_1[bit_len];
	convertUnit(unitInUs, bit_len, bit0_pulses, us_0);
	convertUnit(unitInUs, bit_len, bit1_pulses, us_1);
	bit = new SingleBitPartHandler(us_0, us_1, bit_len, bit_count);

	PartHandler *suffix;
	if (suffix_pulses > 0) {
		uint16_t us[suffix_len];
		convertUnit(unitInUs, suffix_len, suffix_pulses, us);
		suffix = new FixedPartHandler(us, suffix_len);
	} else
		suffix = NULL;

	ComposedProtocolHandler *handler = new ComposedProtocolHandler(id, pre, bit, suffix, repeats);
	_rc->addProtocol(handler);
	return true;
}

void IRRFReceiver::removeProtocol(uint8_t id) {
	SingleTypeRemoteControlProtocolHandler **protocols;
	uint8_t count;
	_rc->getProtocols((RemoteControlProtocolHandler***)&protocols, &count);

	for (uint8_t i=0; i<count; i++) {
		if (protocols[i]->getType() == id) {
			_rc->removeProtocol(protocols[i]);
			free(protocols[i]);
		}
	}
}

void IRRFReceiver::removeAllProtocols() {
	SingleTypeRemoteControlProtocolHandler **protocols;
	uint8_t count;
	_rc->getProtocols((RemoteControlProtocolHandler***)&protocols, &count);

	for (uint8_t i=0; i<count; i++) {
		_rc->removeProtocol(protocols[i]);
		free(protocols[i]);
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


void IRRFReceiver::handleReceivedCode(rc_code code) {
	uint8_t data[9]; // 4 + 1 + 4
	fillPublishHeader(data, 0x0001);
	data[4] = code.type;
	data[5] = (code.code >> 24) & 0xFF;
	data[6] = (code.code >> 16) & 0xFF;
	data[7] = (code.code >> 8) & 0xFF;
	data[8] = code.code & 0xFF;

	for (uint8_t i=0; i<_subscriber_count; i++) {
		_sender->send(_subscribers[i], data, 9);
	}

#ifdef DEBUG_IRRF
	Serial.print("irrf: sent received command ");
	Serial.print(code.type, HEX);
	Serial.print(" - ");
	Serial.print(code.code, HEX);
	Serial.print(" to ");
	Serial.print(_subscriber_count, 10);
	Serial.println(" subscribers.");
#endif
}

