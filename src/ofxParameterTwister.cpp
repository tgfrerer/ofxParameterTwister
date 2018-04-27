#include "ofxParameterTwister.h"

#include "ofMath.h"

#include "ofThreadChannel.h"
#include "ofParameter.h"

#include "RtMidi.h"
#include <array>

using namespace std;

static const std::string MIDI_DEVICE_NAME("Midi Fighter Twister");

/* We assume high resolution encoders with 14 bit resolution */
#define MAX_ENCODER_VALUE (0x3FFF)

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

// ------------------------------------------------------
/// \brief		static callback for midi controller
/// \detail		Translates the message into a midi messge object
///             and passes it on to a thread channel so it can be processed 
///             in update.
/// \note 
static void _midi_callback(double deltatime, std::vector< unsigned char > *message, void *threadChannel)
{

	auto tCh = static_cast<ofThreadChannel<MidiCCMessage>*>(threadChannel);

	// Default midi messages come in three bytes - any other messages will be ignored.

	if (message->size() == 3) {

		MidiCCMessage msg;
		msg.command_channel = message->at(0);
		msg.controller      = message->at(1);
		msg.value           = message->at(2);

#ifndef NDEBUG
		ostringstream ostr;
		ostr
			<< "midi message: "
			<< "0x" << std::hex << std::setw(2) << 1 * message->at(0) << " "
			<< "0x" << std::hex << std::setw(2) << 1 * message->at(1) << " "
			<< "0x" << std::hex << std::setw(2) << 1 * message->at(2)
			;
		ofLogVerbose() << ostr.str();
#endif
		
		tCh->send(std::move(msg));

	}
}

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
	// may be (0..3FFF) == (0..2^14-1) == (0..16383)

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

// Implementation for paramter twister - any calls to twister will be forwared to 
// an implementation instance.

class ofxParameterTwisterImpl {
	
	RtMidiIn*	mMidiIn = nullptr;
	RtMidiOut*	mMidiOut = nullptr;

	ofThreadChannel<MidiCCMessage> mChannelMidiIn;

	ofParameterGroup mParams;

	std::array<Encoder, 16> mEncoders;
public:
	void setup();
	void setParams(const ofParameterGroup& group_);
	void update();
	~ofxParameterTwisterImpl();
};


// ------------------------------------------------------

ofxParameterTwister::ofxParameterTwister() {
}

// ------------------------------------------------------

ofxParameterTwister::~ofxParameterTwister() {
}

// ------------------------------------------------------

void ofxParameterTwister::setup() {
	impl = std::make_unique<ofxParameterTwisterImpl>();
	impl->setup();
}

// ------------------------------------------------------

void ofxParameterTwister::setParams(const ofParameterGroup& group_) {
	
	if (impl) {
		impl->setParams(group_);
	}
	else {
		ofLogWarning() << "ofxParameterTwister::" << __func__ << "() : setup() must be called before calling " << __func__ << " for the first time. Calling setup implicitly...";
		setup();
		// call this method again.
		setParams(group_);
	}
	
}

// ------------------------------------------------------

void ofxParameterTwister::update() {

	if (impl) {
		impl->update();
	}
	else {
		ofLogWarning() << "ofxParameterTwister::" << __func__ << "() : setup() must be called before calling " << __func__ << " for the first time. Calling setup implicitly...";
		setup();
		// call this method again.
		update();
	}

}

// ------------------------------------------------------

void Encoder::setState(State s_, bool force_) {
	if (s_ == mState && force_ == false) {
		return;
	}

	// ----------| invariant: state change requested, or forced

	switch (s_)
	{
	case Encoder::State::DISABLED:
		setEncoderAnimation(0);
		// we need to switch off the status LED
		sendToSwitch(0);
		// we need to switch off the rotary status LED
		sendToRotary(0);
		setBrightnessRotary(0.f);
		setBrightnessRGB(1.f);
		break;
	case Encoder::State::ROTARY:
		sendToSwitch(0);
		setBrightnessRotary(1.f);
		setBrightnessRGB(0.0f);
		break;
	case Encoder::State::SWITCH:
		sendToRotary(0);
		setBrightnessRotary(0.f);
		setBrightnessRGB(1.0f);
		setEncoderAnimation(65);
		break;
	default:
		break;
	}

	mState = s_;
}

// ------------------------------------------------------

void Encoder::setValue(uint8_t v_) {

	switch (mState)
	{
	case Encoder::State::DISABLED:
		ofLogError() << "cannot send value to diabled encoder" << pos;
		break;
	case Encoder::State::ROTARY:
		sendToRotary(v_);
		break;
	case Encoder::State::SWITCH:
		sendToSwitch(v_);
		break;
	default:
		break;
	}

}

// ------------------------------------------------------

void Encoder::sendToSwitch(uint8_t v_) {
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	vector<unsigned char> msg{
		0xB1,					// // `B` means "Control Change" message, lower nibble means channel id; SWITCH listens on channel 1
		pos,					// device id
		v_,						// value
	};
	mMidiOut->sendMessage(&msg);

	ofLogVerbose() << ">>" << setw(2) << 1 * pos << " SWI " << " : " << setw(3) << v_ * 1;
}

// ------------------------------------------------------

void Encoder::sendToRotary(uint8_t v_) {
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	vector<unsigned char> msg{
		0xB0,					// `B` means "Control Change" message, lower nibble means channel id; ROTARY listens on channel 0
		pos,					// device id
		v_,						// value
	};

	mMidiOut->sendMessage(&msg);

	ofLogVerbose() << ">>" << setw(2) << 1 * pos << " ROT " << " : " << setw(3) << v_ * 1;
}

// ------------------------------------------------------

void Encoder::setBrightnessRotary(float b_)
{
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	unsigned char val = std::round(ofMap(b_, 0.f, 1.f, 65, 95, true));
	
	vector<unsigned char> msg{
		0xB2,					// `B` means "Control Change" message, lower nibble means channel id; ANIMATIONS are set via channel 2
		pos,					// device id
		val,
	};

	mMidiOut->sendMessage(&msg);

}

// ------------------------------------------------------

void Encoder::setBrightnessRGB(float b_)
{
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	unsigned char val = round(ofMap(b_, 0.f, 1.f, 17, 47, true));

	vector<unsigned char> msg{
		0xB2,					// // `B` means "Control Change" message, lower nibble means channel id; ANIMATIONS are set via channel 2 
		pos,					// device id
		val,
	};

	mMidiOut->sendMessage(&msg);

}
// ------------------------------------------------------

void Encoder::setEncoderAnimation(uint8_t v_)
{
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	vector<unsigned char> msg{
		0xB2,					// `B` means "Control Change" message, lower nibble means channel id; ANIMATIONS are set via channel 2
		pos,					// device id
		v_,
	};

	mMidiOut->sendMessage(&msg);

}

// ------------------------------------------------------

ofxParameterTwisterImpl::~ofxParameterTwisterImpl() {
	if (mMidiIn != nullptr) {
		mMidiIn->closePort();
		delete mMidiIn;
		mMidiIn = nullptr;
	}
	if (mMidiOut != nullptr) {
		mMidiOut->closePort();
		delete mMidiOut;
		mMidiOut = nullptr;
	}
}

// ------------------------------------------------------

void ofxParameterTwisterImpl::setup() {
	// establish midi in connection,
	// and bind callback for midi in.
	try {
		mMidiIn = new RtMidiIn();
		size_t numPorts = mMidiIn->getPortCount();
		size_t midiPort = -1;
		
		if (mMidiIn->getPortCount() >= 1)
		{
			for (size_t i = 0; i < numPorts; ++i)
			{
				if (mMidiIn->getPortName(i).substr(0, MIDI_DEVICE_NAME.size()) == MIDI_DEVICE_NAME)
				{
					midiPort = i;
					mMidiIn->openPort(midiPort);
					mMidiIn->setCallback(&_midi_callback, &mChannelMidiIn);

					// Don't ignore sysex, timing, or active sensing messages.
					mMidiIn->ignoreTypes(true, true, true);
				}
			}
		}
	}
	catch (RtMidiError &error)
	{
		std::cout << "MIDI input exception:" << std::endl;
		error.printMessage();
	}

	// establish midi out connection
	try {
		mMidiOut = new RtMidiOut();
		size_t numPorts = mMidiOut->getPortCount();
		size_t midiPort = -1;
		if (mMidiOut->getPortCount() >= 1)
		{
			for (size_t i = 0; i < numPorts; ++i)
			{
				if (mMidiOut->getPortName(i).substr(0, MIDI_DEVICE_NAME.size()) == MIDI_DEVICE_NAME)
				{
					midiPort = i;
					mMidiOut->openPort(midiPort);
				}
			}
		}
	}
	catch (RtMidiError &error)
	{
		std::cout << "MIDI output exception:" << std::endl;
		error.printMessage();
	}

	// assign ids to encoders
	for (int i = 0; i < 16; ++i) {
		mEncoders[i].pos = i;
		mEncoders[i].mMidiOut = mMidiOut;
	};
}

// ------------------------------------------------------

void ofxParameterTwisterImpl::setParams(const ofParameterGroup & group_) {
	ofLogVerbose() << "Updating mapping" << endl;
	/*

	based on incoming parameters,
	we set our Encoders to track them

	*/

	auto it = group_.begin();
	auto endIt = group_.end();

	for (auto & e : mEncoders) {

		if (it != endIt) {
			if (auto param = dynamic_pointer_cast<ofParameter<float>>(*it)) {
				// bingo, we have a float param
				e.setState(Encoder::State::ROTARY);
				e.setValue(ofMap(*param, param->getMin(), param->getMax(), 0, 127, true));

				// now set the Encoder's event listener to track 
				// this parameter
				auto pMin = param->getMin();
				auto pMax = param->getMax();

				e.updateParameter = [=](uint8_t v_) {
					// on midi input
					param->set(ofMap(v_, 0, 127, pMin, pMax, true));
				};

				e.mELParamChange = param->newListener([&e, pMin, pMax](float v_) {
					// on parameter change, write from parameter 
					// to midi.
					e.setValue(ofMap(v_, pMin, pMax, 0, 127, true));
				});

			}
			else if (auto param = dynamic_pointer_cast<ofParameter<bool>>(*it)) {
				// we have a bool parameter
				e.setState(Encoder::State::SWITCH);
				e.setValue((*param == true) ? 127 : 0);

				e.updateParameter = [=](uint8_t v_) {
					param->set((v_ > 63) ? true : false);
				};

				e.mELParamChange = param->newListener([&e](bool v_) {
					e.setValue(v_ == true ? 127 : 0);
				});

			}
			else {
				// we cannot match this parameter, unfortunately
				e.setState(Encoder::State::DISABLED);
				e.mELParamChange.unsubscribe(); // reset listener
			}

			it++;

		}
		else {
			// no more parameters to map.
			e.setState(Encoder::State::DISABLED, true);
		}
	}
}

// ------------------------------------------------------

void ofxParameterTwisterImpl::update() {
	MidiCCMessage m;

	while (mChannelMidiIn.tryReceive(m)) {

		// we got a message.
		// let's get the address.

		if (m.getCommand() == 0xB) {

			if (m.getChannel() == 0x0) {
				// rotary message
				size_t encoderID = m.controller;
				auto &e = mEncoders[encoderID];
				if (e.mState == Encoder::State::ROTARY)
					if (e.updateParameter)
						e.updateParameter(m.value);
			}

			if (m.getChannel() == 0x1) {
				// rotary message
				size_t encoderID = m.controller;
				auto &e = mEncoders[encoderID];
				if (e.mState == Encoder::State::SWITCH)
					if (e.updateParameter)
						e.updateParameter(m.value);
			}
		}
	}
}

// ------------------------------------------------------
