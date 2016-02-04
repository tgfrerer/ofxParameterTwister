#pragma once

#include <memory>

#include "ofThreadChannel.h"
#include "ofParameter.h"

class RtMidiIn; // ffdecl.
class RtMidiOut; // ffdecl.

class ofAbstractParameter;
/*

+ internally, we hold a parametergroup

  + we can set a parameter group
  + re can clear a parameter group

  up to 16 parameters from the parameter group
  will get bound to twister

  floats and ints will get bound to knobs
  bool will get bound to button

  button state is represented using color

  parameter change outside of twister is sent to twister.



*/
#include <cstdint> ///< we include this to get access to standard sized types

namespace pal {
namespace Kontrol {

struct MidiCCMessage {
	uint8_t command_channel		= 0xB0;
	uint8_t controller			= 0x00;
	uint8_t value				= 0x00;
	
	int getCommand() {
		// command is in the most significant 
		// 4 bits, so we shift 4 bits to the right.
		// e.g. 0xB0
		return command_channel >> 4;
	};

	int getChannel() {
		// channel is the least significant 4 bits,
		// so we null out the high bits
		return command_channel & 0x0F;
	};
	
};

struct AbstractMidiParam {
	virtual ~AbstractMidiParam() {};
	virtual vector<unsigned char> getMessage() = 0;
};

template <typename T>
struct MidiParam : public AbstractMidiParam {
	uint16_t mAddr			= 0xB000;
	ofParameter<T>* mParam	= nullptr;
	RtMidiOut* mMidiOut		= nullptr;
	
	// note that the implementation does 
	// not use templates, but only two specialisations (bool / float)
	// as we're perfectly happy to limit ourselves
	// to two different parameter types for now.
	void valueChanged(T &value);
	
	vector<unsigned char> getMessage() override;
};

class MidiFighter
{
	std::unordered_map<uint16_t, std::unique_ptr<AbstractMidiParam>> mMidiParams;
	ofParameterGroup mParamGroup;

public:

	MidiFighter();
	~MidiFighter();

	void setup();
	void update(); // this is where we apply values.

	void setParams(const ofParameterGroup& group_);

private:

	RtMidiIn*	mMidiIn = nullptr;
	RtMidiOut*	mMidiOut = nullptr;

	ofThreadChannel<MidiCCMessage> mTch;
	unordered_map<unsigned char, ofAbstractParameter*> mParams;
};

} // close namespace Kontrol
} // close namespace pal

