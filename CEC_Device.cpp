#include "CEC_Device.h"

CEC_Device::CEC_Device() :
	_monitorMode(true),
	_promiscuous(false),
	_logicalAddress(-1),
	_state(CEC_IDLE),
	_receiveBufferBits(0),
	_transmitBufferBytes(0),
	_amLastTransmittor(false),
	_bitStartTime(0),
	_waitTime(0)
{
}

void CEC_Device::Initialize(int physicalAddress, CEC_DEVICE_TYPE type, bool promiscuous, bool monitorMode)
{
	static const char valid_LogicalAddressesTV[3]    = {CLA_TV, CLA_FREE_USE, CLA_UNREGISTERED};
	static const char valid_LogicalAddressesRec[4]   = {CLA_RECORDING_DEVICE_1, CLA_RECORDING_DEVICE_2,
	                                                    CLA_RECORDING_DEVICE_3, CLA_UNREGISTERED};
	static const char valid_LogicalAddressesPlay[4]  = {CLA_PLAYBACK_DEVICE_1, CLA_PLAYBACK_DEVICE_2,
	                                                    CLA_PLAYBACK_DEVICE_3, CLA_UNREGISTERED};
	static const char valid_LogicalAddressesTuner[5] = {CLA_TUNER_1, CLA_TUNER_2, CLA_TUNER_3,
	                                                    CLA_TUNER_4, CLA_UNREGISTERED};
	static const char valid_LogicalAddressesAudio[2] = {CLA_AUDIO_SYSTEM, CLA_UNREGISTERED};

	switch(type) {
	case CDT_TV:               _validLogicalAddresses = valid_LogicalAddressesTV;    break;
	case CDT_RECORDING_DEVICE: _validLogicalAddresses = valid_LogicalAddressesRec;   break;
	case CDT_PLAYBACK_DEVICE:  _validLogicalAddresses = valid_LogicalAddressesPlay;  break;
	case CDT_TUNER:            _validLogicalAddresses = valid_LogicalAddressesTuner; break;
	case CDT_AUDIO_SYSTEM:     _validLogicalAddresses = valid_LogicalAddressesAudio; break;
	default:                   _validLogicalAddresses = NULL;
	}

	_promiscuous = promiscuous;
	_monitorMode = monitorMode;
	_physicalAddress = physicalAddress & 0xffff;
	_logicalAddress = -1;

	// <Polling Message> to allocate a logical address when physical address is valid
	if (_validLogicalAddresses && _physicalAddress != 0xffff)
		Transmit(*_validLogicalAddresses, *_validLogicalAddresses, NULL, 0);
}

///
/// CEC_Device::Run implements our main state machine
/// which includes all reading and writing of state including
/// acknowledgements and arbitration
///

void CEC_Device::Run()
{
	bool currentLineState = LineState();
	unsigned long time = micros();
	unsigned long difftime = time - _bitStartTime;
	if (currentLineState == _lastLineState && _state != CEC_IDLE &&
	    (_waitTime == (unsigned int)-1 || _waitTime > difftime))
		// No line transition and wait for external event, or wait time not elapsed; nothing to do
		// In IDLE state we need to check for pending transmit, though
		return;

	if (currentLineState != _lastLineState &&
	    _state >= CEC_XMIT_WAIT && _state != CEC_XMIT_ACK_TEST && _state != CEC_XMIT_ACK_WAIT)
		// We are in a transmit state and someone else is mucking with the line
		// Try to receive and wait for the line to clear before (re)transmit
		// However, it is OK for a follower to ACK if we are in an ACK state
		_state = CEC_IDLE;

	bool bit;
	_waitTime = (unsigned int)-1;	// INFINITE by default; (== wait until an external event has occurred)
	switch (_state) {
	case CEC_IDLE:
		// If a high to low transition occurs, this must be the beginning of a start bit
		if (!currentLineState) {
			_receiveBufferBits = 0;
			_bitStartTime = time;
			_ack = true;
			_follower = false;
			_broadcast = false;
			_amLastTransmittor = false;
			_state = CEC_RCV_STARTBIT1;
		} else if (_transmitBufferBytes)
			// Transmit pending
			if (_xmitretry > CEC_MAX_RETRANSMIT)
				// No more
				_transmitBufferBytes = 0;
			else {
				// We need to wait a certain amount of time before we can transmit
				_waitTime = ((_xmitretry) ?  3 * BIT_TIME :
				             (_amLastTransmittor) ? 7 * BIT_TIME :
				             5 * BIT_TIME);
				_state = CEC_XMIT_WAIT;
			}
		// Nothing to do until we have a need to transmit
		// or we detect the falling edge of the start bit
		break;

	case CEC_RCV_STARTBIT1:
		// We're waiting for the rising edge of the start bit
		if (difftime >= (S                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               