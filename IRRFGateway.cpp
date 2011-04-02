#include "IRRFGateway.h"
#include "ComposedProtocolHandler.h"

void handle_rcrf(rc_code code, void *object) {
	IRRFGateway *receiver = (IRRFGateway*)object;
	receiver->handleReceivedCode(code);
}

IRRFGateway::IRRFGateway(RemoteController *rc) {
	_rc = rc;
	_rc->setHandler(handle_rcrf, this);
	_type = 0x10;
	_version = 0x00;
	_subscription = new SubscriptionManager(_sender);
}

void IRRFGateway::processCall(uint16_t callType, XBeeAddress from, uint8_t* payload, uint8_t payload_length) {
	switch (callType) {
	case 0x0001: { //unload
		if (payload_length < 1) break;
		removeProtocol(payload[0]);
		break;
	}
	case 0x0002: { // unload all
		removeAllProtocols();
		break;
	}
	case 0x0010: { // send command
		if (payload_length < 9) break;
		uint64_t cmd = 0;
		for (int i=1; i<9; i++)
			cmd = (cmd << 8) | payload[i];
		sendCommand(payload[0], cmd);
	}
	default: break;
	}
}

bool IRRFGateway::processRequest(uint16_t requestType, XBeeAddress from, uint8_t* payload, uint8_t payload_length) {
	switch (requestType) {
	case 0x0001: {// load single-bit fixed-length
		if (payload_length < 9) return false;
		if (addSingleFixedProtocol(payload[0], &payload[1], payload_length-1)) {
			//ok
			uint8_t data[] = { 0, 0, 0, 0, payload[0], 0x01 };
			fillResponseHeader(data, requestType);
			_sender->send(from, data, 6);
		} else {
			//failed
			uint8_t data[] = { 0, 0, 0, 0, payload[0], 0xFF };
			fillResponseHeader(data, requestType);
			_sender->send(from, data, 6);
		}
		return true;
	}
	default:
		return false;
	}
}

bool IRRFGateway::addSubscription(XBeeAddress from, uint16_t subscriptionType) {
	switch (subscriptionType) {
	case 0x0001: // detected ir commands
		_subscription->add(from);
		return true;
	default:
		return false;
	}
}

void IRRFGateway::removeSubscription(XBeeAddress from, uint16_t subscriptionType) {
	switch (subscriptionType) {
	case 0001: // detected ir commands
		_subscription->remove(from);
		break;
	default: break;
	}
}

void IRRFGateway::sendCommand(uint8_t protocol_id, uint64_t code) {
#ifdef DEBUG_IRRF
		Serial.print("irrfr: sending irrf command ");
		Serial.print((uint32_t)code, 10);
		Serial.print(" (protocol-id ");
		Serial.print(protocol_id, 10);
		Serial.println(")");
#endif
		rc_code rc;
		rc.type = protocol_id;
		rc.code = code;
		_rc->send_code(rc);
}

void convertUnit(uint16_t unitInUs, uint8_t count, uint8_t *data, uint16_t *out) {
	for (uint8_t i=0; i<count; i++)
		out[i] = unitInUs * data[i];
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
bool IRRFGateway::addSingleFixedProtocol(uint8_t id, uint8_t *data, uint8_t len) {
	uint16_t unitInUs = ((uint16_t)data[0]) << 8 | data[1];
	uint8_t repeats = data[2];
	uint8_t pre_len = data[3];
	uint8_t bit_len = data[4];
	uint8_t suffix_len = data[5];
	uint8_t bit_count = data[6];
	if (pre_len+bit_len*2+suffix_len+7 != len) {
#ifdef DEBUG_IRRF
		Serial.print("irrfr: addsfp: malformed (");
		Serial.print(len, 10);
		Serial.print("!=");
		Serial.print(pre_len+bit_len*2+suffix_len+7);
		Serial.println(")");
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

void IRRFGateway::removeProtocol(uint8_t id) {
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

void IRRFGateway::removeAllProtocols() {
	SingleTypeRemoteControlProtocolHandler **protocols;
	uint8_t count;
	_rc->getProtocols((RemoteControlProtocolHandler***)&protocols, &count);

	for (uint8_t i=0; i<count; i++) {
		_rc->removeProtocol(protocols[i]);
		free(protocols[i]);
	}
}

void IRRFGateway::handleReceivedCode(rc_code code) {
	uint8_t data[13]; // 4 + 1 + 8
	fillPublishHeader(data, 0x0001);
	data[4] = code.type;
	data[5] = 0;
	data[6] = 0;
	data[7] = 0;
	data[8] = 0;
	data[9] = (code.code >> 24) & 0xFF;
	data[10] = (code.code >> 16) & 0xFF;
	data[11] = (code.code >> 8) & 0xFF;
	data[12] = code.code & 0xFF;

	_subscription->publish(data, 13);

#ifdef DEBUG_IRRF
	Serial.print("irrf: sent received command ");
	Serial.print(code.type, HEX);
	Serial.print(" - ");
	Serial.print(code.code, HEX);
	Serial.println(" to subscribers.");
#endif
}

