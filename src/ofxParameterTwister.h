#pragma once

#include <memory>
#include <array>
#include "ofThreadChannel.h"
#include "ofParameter.h"
#include "RtMidi.h"


class ofAbstractParameter;
/*

+ internally, we hold a parametergroup

  + we can set a parameter group
  + re can clear a parameter group

  up to 16 parameters from the parameter group
  bind/unbind automatically to twister:

  float -> rotary controller
  bool  -> switch (button)

  button state is represented using RGB color.
  by default, buttons act as toggles.

  parameter change outside of twister is sent to twister
  whilst parameters are bound to twister.

*/
#include <cstdint> ///< we include this to get access to standard sized types

namespace pal {
namespace Kontrol {

struct MidiCCMessage {
	uint8_t command_channel = 0xB0;
	uint8_t controller = 0x00;
	uint8_t value = 0x00;

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




class ofxParameterTwister
{

	struct Encoder {

		RtMidiOut*	mMidiOut = nullptr;

		// position on the controller left to right,
		// top to bottom
		uint8_t pos = 0;

		// knob may be either 
		// disabled, or a rotary controller, or a switch.
		enum class State {
			DISABLED,
			ROTARY,
			SWITCH
		} mState = State::DISABLED;

		// internal representation of the knob value 
		// may be 0..127
		uint8_t value = 0;

		// event listener for parameter change
		ofEventListener mELParamChange;

		std::function<void(uint8_t v_)> updateParameter;

		void setState(State s_, bool force_ = false);
		void setValue(uint8_t v_);

		void sendToSwitch(uint8_t v_);
		void sendToRotary(uint8_t v_);

		void setEncoderAnimation(uint8_t v_);
		void setBrightnessRotary(float b_); /// brightness is normalised over 31 steps 0..30
		void setBrightnessRGB(float b_);
	};


public:
	
	
	~ofxParameterTwister();

	void setup();
	void clear();

	void update(); // this is where we apply values.

	void setParams(const ofParameterGroup& group_);
	void setParam(size_t idx_, ofParameter<float>& param_);
	void setParam(size_t idx_, ofParameter<bool>& param_);

	void clearParam(size_t idx_, bool force_ = false);

private:

	void setParam(Encoder& encoder_, ofParameter<float>& param_);
	void setParam(Encoder& encoder_, ofParameter<bool>& param_);

	void clearParam(Encoder& encoder_, bool force_);

	RtMidiIn*	mMidiIn = nullptr;
	RtMidiOut*	mMidiOut = nullptr;

	ofThreadChannel<MidiCCMessage> mChannelMidiIn;

	ofParameterGroup mParams;

	std::array<ofxParameterTwister::Encoder, 16> mEncoders;

};

} // close namespace Kontrol
} // close namespace pal

