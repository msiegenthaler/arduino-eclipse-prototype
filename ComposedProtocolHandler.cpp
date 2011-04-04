#include "ComposedProtocolHandler.h"

ComposedProtocolHandler::ComposedProtocolHandler(uint8_t type, PartHandler *pre, DataPartHandler *data, PartHandler *post, uint8_t repeats) {
	_type = type;
	_pre = pre;
	_data = data;
	_post = post;
	_repeats = repeats;
	_repeats_to_trigger = max(1, round(((float)repeats) * 2 / 3));
	_state = CPH_STATE_INIT;
	_handler = NULL;
	_handler_object = NULL;
	_candidate = 0;
	_candidate_count = 0;
	_candidate_t = 0;
}

uint8_t ComposedProtocolHandler::getType() {
	return _type;
}

void ComposedProtocolHandler::setHandler(void (*handler_fun)(rc_code, void*), void *object) {
	_handler = handler_fun;
	_handler_object = object;
}

void ComposedProtocolHandler::processPulse(uint16_t high, uint16_t low) {
#ifdef DEBUG_PROTOCOL_FINEST
	if (_state != CPH_STATE_INIT) {
		Serial.print("CPH: process ");
		Serial.print(high, 10);
		Serial.print(" / ");
		Serial.println(low, 10);
	}
#endif
	handlePulse(high);
	if (_state != CPH_STATE_INIT) //always start a sequence with a high
		handlePulse(low);
}

void ComposedProtocolHandler::handlePulse(uint16_t pulse) {
	switch(_state) {
	case CPH_STATE_INIT:
#ifdef DEBUG_PROTOCOL_FINEST
			Serial.print("'");
#endif
		if (_pre) {
			_pre->reset();
			_state = CPH_STATE_PRE;
		} else _state = CPH_STATE_DATA;
		//no break, handle as pre
	case CPH_STATE_PRE:
		switch (_pre->processPulse(pulse)) {
		case PH_RESULT_MORE:
#ifdef DEBUG_PROTOCOL_FINEST
			Serial.print(".");
#endif
			break;
		case PH_RESULT_OK:
#ifdef DEBUG_PROTOCOL_FINEST
			Serial.println("CPH: pre ok");
#endif
			_state = CPH_STATE_DATA;
			_data->reset();
			break;
		case PH_RESULT_ABORT:
#ifdef DEBUG_PROTOCOL_FINEST2
			Serial.print("CPH: pre failed (");
			Serial.print(pulse);
			Serial.println(")");
#endif
			_state = CPH_STATE_INIT;
			break;
		}
		break;

	case CPH_STATE_DATA:
#ifdef DEBUG_PROTOCOL_FINEST
		Serial.print("-");
#endif
		switch (_data->processPulse(pulse)) {
		case PH_RESULT_MORE: break;
		case PH_RESULT_OK:
#ifdef DEBUG_PROTOCOL_FINE
			Serial.print("CPH: data ok: ");
			Serial.println(_data->value(), 2);
#endif
			if (_post) {
				_state = CPH_STATE_POST;
				_post->reset();
			} else {
				handleCandidate(_data->value());
				_state = CPH_STATE_INIT;
			}
			break;
		case PH_RESULT_ABORT:
#ifdef DEBUG_PROTOCOL_FINE
			Serial.print("CPH: data failed (");
			Serial.print(pulse);
			Serial.println(")");
#endif
			_state = CPH_STATE_INIT;
			break;
		}
		break;

	case CPH_STATE_POST:
#ifdef DEBUG_PROTOCOL_FINEST
			Serial.print("+");
#endif
		switch (_post->processPulse(pulse)) {
		case PH_RESULT_MORE: break;
		case PH_RESULT_OK:
#ifdef DEBUG_PROTOCOL_FINE
			Serial.println("CPH: post ok");
#endif
			handleCandidate(_data->value());
			_state = CPH_STATE_INIT;
			break;
		case PH_RESULT_ABORT:
#ifdef DEBUG_PROTOCOL_FINE
			Serial.print("CPH: post failed (");
			Serial.print(pulse);
			Serial.println(")");
#endif
			_state = CPH_STATE_INIT;
			break;
		}
		break;
	}
}

#define REPEAT_TIMEOUT 200; // 200ms

void ComposedProtocolHandler::handleCandidate(uint32_t candidate) {
#ifdef DEBUG_PROTOCOL
	Serial.print("Found candidate 0x");
	Serial.print(candidate, HEX);
	Serial.print(" for type ");
	Serial.println(_type, HEX);
#endif

	uint16_t t = millis();
	bool timedout = t > _candidate_t + REPEAT_TIMEOUT;

	// calculate the new _candidate_count
	if (_candidate_count > 0 && !timedout) {
		if (_candidate == candidate) {
			_candidate_count++;
		} else {
			//New candidate
			_candidate = candidate;
			_candidate_count = 1;
			_candidate_t = t;
		}
	} else {
		//New candiate
		_candidate = candidate;
		_candidate_count = 1;
		_candidate_t = t;
	}

	if (_candidate_count >= _repeats && _repeats > 1) {
		// we reached the expected count, reset the repeats to zero to be ready
		// for the next one
		_candidate_count = 0;
		_candidate = 0;
		_candidate_t = 0;
	} else if (_candidate_count == _repeats_to_trigger) {
		// we reached the trigger-limit.. so trigger
		rc_code code;
		code.type = _type;
		code.code = candidate;
		if (_handler) _handler(code, _handler_object);
	}
}

PulseIterator* ComposedProtocolHandler::makePulseIterator(rc_code code) {
	PulseIterator **iterators = (PulseIterator**)malloc(sizeof(PulseIterator*)*3);
	uint8_t index = 0;
	if (_pre) iterators[index++] = _pre->pulseIterator(code.code);
	iterators[index++] = _data->pulseIterator(code.code);
	if (_post) iterators[index++] = _post->pulseIterator(code.code);
	return new ComposedPulseIterator(iterators, index, _repeats*10); //TODO
}


//Utility Functions

uint16_t* copyDataArray(uint16_t *source, byte len) {
	size_t size = sizeof(uint16_t) * len;
	uint16_t* result = (uint16_t*) malloc(size);
	memcpy(result, source, size);
	return result;
}
bool isPulseUs(uint16_t pulse, uint16_t check) {
	if (check == 0) return true; // if 0 is expected then everything matches
	int16_t value = ((int16_t) pulse) - check;
	return abs(value) < ((int16_t)check / 3); //33% tolerance
}

//////////////////////////
// Part Handlers
//////////////////////////

//Fixed

FixedPartHandler::FixedPartHandler(uint16_t *pulses, uint8_t count) {
	_pulses = copyDataArray(pulses, count);
	_pulse_count = count;
	reset();
}

void FixedPartHandler::reset() {
	_pos = 0;
}

PHResult FixedPartHandler::processPulse(uint16_t pulse) {
	if (isPulseUs(pulse, _pulses[_pos])) {
		//match
		_pos++;
		if (_pos >= _pulse_count) return PH_RESULT_OK;
		else return PH_RESULT_MORE;
	} else {
		//no match
		return PH_RESULT_ABORT;
	}
}

PulseIterator* FixedPartHandler::pulseIterator(uint32_t value) {
	return new FixedPulseIterator(_pulses, _pulse_count);
}

//Single Bit

SingleBitPartHandler::SingleBitPartHandler(uint16_t *zero_pulses, uint16_t *one_pulses, uint8_t count, uint8_t bit_count) {
	_pulses_zero = copyDataArray(zero_pulses, count);
	_pulses_one = copyDataArray(one_pulses, count);
	_pulse_count = count;
	_bit_count = bit_count;
	reset();
}

void SingleBitPartHandler::reset() {
	_pos = 0;
	_maybe_one = true;
	_maybe_zero = true;
	_value = 0;
	_value_bits = 0;
}

PHResult SingleBitPartHandler::processPulse(uint16_t pulse) {
	if (_maybe_zero && !isPulseUs(pulse, _pulses_zero[_pos])) _maybe_zero = false;
	if (_maybe_one && !isPulseUs(pulse, _pulses_one[_pos])) _maybe_one = false;

	if (!_maybe_zero && !_maybe_one) return PH_RESULT_ABORT;
	_pos++;
	if (_pos >= _pulse_count) {
		_value = _value << 1;
		if (_maybe_one) _value |= 1;
#ifdef DEBUG_PROTOCOL_FINEST
		Serial.print("CPH: detected ");
		if (_maybe_one) Serial.println("1");
		else Serial.println("0");
#endif
		_value_bits++;
		if (_value_bits >= _bit_count) return PH_RESULT_OK;
		else {
			_pos = 0;
			_maybe_one = true;
			_maybe_zero = true;
			return PH_RESULT_MORE;
		}
	} else return PH_RESULT_MORE;
}

uint32_t SingleBitPartHandler::value() {
	return _value;
}


PulseIterator* SingleBitPartHandler::pulseIterator(uint32_t value) {
	return new BitPulseIterator(_pulses_zero, _pulses_one, _pulse_count, _bit_count, value);
}



FixedPulseIterator::FixedPulseIterator(uint16_t *pulses, uint8_t count) {
	_pulses = pulses;
	_pulse_count = count;
	_pos = 0;
}

bool FixedPulseIterator::nextPulse(uint16_t *pulseUs) {
	*pulseUs = _pulses[_pos++];
	if (_pos < _pulse_count) return true;
	else {
		_pos = 0;
		return false;
	}
}


BitPulseIterator::BitPulseIterator(uint16_t *zero_pulses, uint16_t *one_pulses, uint8_t count, uint8_t bit_count, uint32_t value) {
	_pulses_zero = zero_pulses;
	_pulses_one = one_pulses;
	_pulse_count = count;
	_bit_count = bit_count;
	_value = value;
	_pos_in_bit = 0;
	_bit = 1;
}

bool BitPulseIterator::nextPulse(uint16_t *pulseUs) {
	bool currentBit = (_value & (1 << (_bit_count - _bit))) != 0;
	uint16_t *pulses;
	if (currentBit) pulses = _pulses_one;
	else pulses = _pulses_zero;

	*pulseUs = pulses[_pos_in_bit];
	if (_pos_in_bit+1 < _pulse_count) {
		_pos_in_bit++;
		return true;
	} else if (_bit < _bit_count) {
		_bit++;
		_pos_in_bit = 0;
		return true;
	} else {
		_pos_in_bit = 0;
		_bit = 1;
		return false;
	}
}

ComposedPulseIterator::ComposedPulseIterator(PulseIterator **iterators, uint8_t count, uint8_t repeats) {
	_iterators = iterators;
	_count = count;
	_pos = 0;
	_repeats = repeats;
	_current_repetition = 1;
}

bool ComposedPulseIterator::nextPulse(uint16_t *pulseUs) {
	if (_iterators[_pos]->nextPulse(pulseUs)) {
		return true;
	} else if (_pos+1 < _count) {
		_pos++;
		return true;
	} else if (_current_repetition < _repeats) {
		_current_repetition++;
		_pos = 0;
		return true;
	} else {
		_current_repetition = 0;
		_pos = 0;
		return false;
	}
}
