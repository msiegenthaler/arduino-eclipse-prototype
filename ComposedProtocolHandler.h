#ifndef COMPOSEDPROTOCOLHANDLER_H_
#define COMPOSEDPROTOCOLHANDLER_H_

#include "RemoteControlProtocolHandler.h"

//#define DEBUG_PROTOCOL
//#define DEBUG_PROTOCOL_FINE
//#define DEBUG_PROTOCOL_FINEST
//#define DEBUG_PROTOCOL_FINEST2

enum PHResult {
	PH_RESULT_MORE,
	PH_RESULT_OK, PH_RESULT_ABORT
};
/**
 * A part of the protocol (i.e. a preample).
 * - reset must be called after an ok or an abort
 * - pulseIterator may be called at any time
 */
class PartHandler {
public:
	virtual void reset();
	virtual PHResult processPulse(uint16_t pulse);
	virtual PulseIterator* pulseIterator(uint32_t forValue);
};
/**
 * A part of the protocol data.
 */
class DataPartHandler : public PartHandler {
public:
	/** only valid if the last call to processPulse returned PH_RESULT_OK */
	virtual uint32_t value();
};


enum CPHState {
	CPH_STATE_INIT, CPH_STATE_PRE, CPH_STATE_DATA, CPH_STATE_POST
};

/**
 * Simple composed protocol handler consisting of:
 *  - preample (optional)
 *  - data
 *  - post (optional)
 * The sequence always starts with a HIGH pulse (first pulse passed to the pre is always a HIGH).
 */
class ComposedProtocolHandler : public SingleTypeRemoteControlProtocolHandler {
public:
	ComposedProtocolHandler(uint8_t type, PartHandler *pre, DataPartHandler *data, PartHandler *post, uint8_t repeats);
	void setHandler(void (*handler_fun)(rc_code, void*), void *object);
	void processPulse(uint16_t pulseHighUs, uint16_t pulseLowUs);
	PulseIterator* makePulseIterator(rc_code code);
	uint8_t getType();
private:
	void handlePulse(uint16_t pulse);
	void handleCandidate(uint32_t candidate);
private:
	uint8_t _type;
	PartHandler *_pre;
	DataPartHandler *_data;
	PartHandler *_post;
	uint8_t _repeats;
	uint8_t _repeats_to_trigger;
	void (*_handler)(rc_code, void*);
	void *_handler_object;
	CPHState _state;
	uint32_t _candidate;
	uint16_t _candidate_t;
	uint8_t _candidate_count;
};


//Part Handlers

/**
 * Expects a fixed sequence of pulses.
 * - pulses with expected length 0 match everything
 */
class FixedPartHandler : public PartHandler {
public:
	FixedPartHandler(uint16_t *pulses, uint8_t count);
	void reset();
	PHResult processPulse(uint16_t pulse);
	PulseIterator* pulseIterator(uint32_t forValue);
private:
	uint16_t *_pulses;
	uint8_t _pulse_count;
	uint8_t _pos;
};

/**
 * Can tell apart pulse seqences representing zero and one.
 * - pulses with expected length 0 match everything
 */
class SingleBitPartHandler : public DataPartHandler {
public:
	SingleBitPartHandler(uint16_t *zero_pulses, uint16_t *one_pulses, uint8_t pulse_count, uint8_t bit_count);
	void reset();
	PHResult processPulse(uint16_t pulse);
	PulseIterator* pulseIterator(uint32_t forValue);
	uint32_t value();
private:
	uint16_t *_pulses_one;
	uint16_t *_pulses_zero;
	uint8_t _pulse_count;
	uint8_t _bit_count;
	uint8_t _pos;
	bool _maybe_one;
	bool _maybe_zero;
	uint32_t _value;
	uint8_t _value_bits;
};


#endif /* COMPOSEDPROTOCOLHANDLER_H_ */
