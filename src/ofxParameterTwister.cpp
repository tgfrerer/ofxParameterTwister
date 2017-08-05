#include "ofxParameterTwister.h"

#include "RtMidi.h"

#include "ofParameter.h"

using namespace pal::Kontrol;

// ------------------------------------------------------
/// \brief		static callback for midi controller
/// \detail		all this callback does is translate the message into a midi messge object
/// and then pass is on to a the midi in thread channel
/// so it can be processed in update.
void _midi_callback(double deltatime, std::vector< unsigned char > *message, void *threadChannel)
{
	unsigned int nBytes = message->size();

	auto tCh = static_cast<ofThreadChannel<MidiCCMessage>*>(threadChannel);

	// message will come in three bytes, with the first byte == 176.

	if (message->size() == 3) {

		MidiCCMessage msg;
		msg.command_channel = message->at(0);
		msg.controller = message->at(1);
		msg.value = message->at(2);

		ostringstream ostr;
		ostr
			<< std::hex << 1 * msg.getCommand() << " : "
			<< std::hex << 1 * msg.getChannel() << " : "
			<< std::hex << 1 * msg.controller << " : "
			<< std::hex << 1 * msg.value;

		ofLogVerbose() << ostr.str();

		tCh->send(std::move(msg));
	}
}

// ------------------------------------------------------

ofxParameterTwister::~ofxParameterTwister() {
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

void ofxParameterTwister::setup() {

	// establish midi in connection,
	// and bind callback for midi in.
	try {
		mMidiIn = new RtMidiIn();
		size_t numPorts = mMidiIn->getPortCount();
		size_t midiPort = -1;
		const std::string deviceName("Midi Fighter Twister");
		if (mMidiIn->getPortCount() >= 1)
		{
			for (size_t i = 0; i < numPorts; ++i)
			{
				if (mMidiIn->getPortName(i).substr(0, deviceName.size()) == deviceName)
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
		const std::string deviceName("Midi Fighter Twister");
		if (mMidiOut->getPortCount() >= 1)
		{
			for (size_t i = 0; i < numPorts; ++i)
			{
				if (mMidiOut->getPortName(i).substr(0, deviceName.size()) == deviceName)
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

void ofxParameterTwister::clear() {
	for (auto & e : mEncoders) {
		clearParam(e, true);
	}
}

// ------------------------------------------------------


// ------------------------------------------------------

void ofxParameterTwister::setParams(const ofParameterGroup& group_) {
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
				setParam(e, *param);

			} else if (auto param = dynamic_pointer_cast<ofParameter<bool>>(*it)) {
				// we have a bool parameter
				setParam(e, *param);

			} else {
				// we cannot match this parameter, unfortunately
				clearParam(e, false);
			}
			
			it++;

		} else {
			// no more parameters to map.
			e.setState(Encoder::State::DISABLED, true);
		}
	}

}

// ------------------------------------------------------

void ofxParameterTwister::setParam(size_t idx_, ofParameter<float>& param_) {
	if (idx_ < mEncoders.size()) {
		setParam(mEncoders[idx_], param_);
	}
}

// ------------------------------------------------------

void ofxParameterTwister::setParam(size_t idx_, ofParameter<bool>& param_) {
	if (idx_ < mEncoders.size()) {
		setParam(mEncoders[idx_], param_);
	}
}

// ------------------------------------------------------

void ofxParameterTwister::setParam(Encoder& encoder_, ofParameter<float>& param_) {
	encoder_.setState(Encoder::State::ROTARY);
	encoder_.setValue(ofMap(param_, param_.getMin(), param_.getMax(), 0, 127, true));

	// now set the Encoder's event listener to track 
	// this parameter
	auto pMin = param_.getMin();
	auto pMax = param_.getMax();

	encoder_.updateParameter = [&param_, pMin, pMax](uint8_t v_) {
		// on midi input
		param_.set(ofMap(v_, 0, 127, pMin, pMax, true));
	};

	encoder_.mELParamChange = param_.newListener([&encoder_, pMin, pMax](float v_) {
		// on parameter change, write from parameter 
		// to midi.
		encoder_.setValue(ofMap(v_, pMin, pMax, 0, 127, true));
	});
}

// ------------------------------------------------------

void ofxParameterTwister::setParam(Encoder& encoder_, ofParameter<bool>& param_) {
	encoder_.setState(Encoder::State::SWITCH);
	encoder_.setValue((param_ == true) ? 127 : 0);

	encoder_.updateParameter = [&param_](uint8_t v_) {
		param_.set((v_ > 63) ? true : false);
	};

	encoder_.mELParamChange = param_.newListener([&encoder_](bool v_) {
		encoder_.setValue(v_ == true ? 127 : 0);
	});
}

// ------------------------------------------------------

void ofxParameterTwister::clearParam(size_t idx_, bool force_) {
	if (idx_ < mEncoders.size()) {
		clearParam(mEncoders[idx_], force_);
	}
}

// ------------------------------------------------------

void ofxParameterTwister::clearParam(Encoder& encoder_, bool force_) {
	encoder_.setState(Encoder::State::DISABLED, force_);
	encoder_.mELParamChange = ofEventListener(); // reset listener
}

// ------------------------------------------------------

void ofxParameterTwister::setHueRGB(size_t idx_, float hue_)
{
	if (idx_ < mEncoders.size()) 
	{
		setHueRGB(mEncoders[idx_], hue_);
	}
}

// ------------------------------------------------------

void ofxParameterTwister::setHueRGB(Encoder& encoder_, float hue_)
{
	encoder_.setHueRGB(hue_);
}

// ------------------------------------------------------

void ofxParameterTwister::setBrightnessRGB(size_t idx_, float bri_)
{
	if (idx_ < mEncoders.size())
	{
		setBrightnessRGB(mEncoders[idx_], bri_);
	}
}

// ------------------------------------------------------

void ofxParameterTwister::setBrightnessRGB(Encoder& encoder_, float bri_)
{
	encoder_.setBrightnessRGB(bri_);
}

// ------------------------------------------------------

void ofxParameterTwister::setAnimationRGB(size_t idx_, Animation anim_, uint8_t rate_)
{
	if (idx_ < mEncoders.size() && rate_ < 8)
	{
		setAnimationRGB(mEncoders[idx_], anim_, rate_);
	}
}

// ------------------------------------------------------

void ofxParameterTwister::setAnimationRGB(Encoder& encoder_, Animation anim_, uint8_t rate_)
{
	uint8_t mode;
	switch (anim_)
	{
	case Animation::Strobe:
		mode = 1 + rate_;
		break;
	case Animation::Pulse:
		mode = 9 + rate_;
		break;
	case Animation::Rainbow:
	default:
		mode = 127;
	}
	encoder_.setAnimation(mode);
}

// ------------------------------------------------------

void ofxParameterTwister::setBrightnessRotary(size_t idx_, float bri_)
{
	if (idx_ < mEncoders.size())
	{
		setBrightnessRotary(mEncoders[idx_], bri_);
	}
}

// ------------------------------------------------------

void ofxParameterTwister::setBrightnessRotary(Encoder& encoder_, float bri_)
{
	encoder_.setBrightnessRotary(bri_);
}

// ------------------------------------------------------

void ofxParameterTwister::setAnimationRotary(size_t idx_, Animation anim_, uint8_t rate_)
{
	if (idx_ < mEncoders.size() && rate_ < 8 && anim_ != Animation::Rainbow)
	{
		setAnimationRotary(mEncoders[idx_], anim_, rate_);
	}
}

// ------------------------------------------------------

void ofxParameterTwister::setAnimationRotary(Encoder& encoder_, Animation anim_, uint8_t rate_)
{
	uint8_t mode;
	switch (anim_)
	{
	case Animation::Strobe:
		mode = 49 + rate_;
		break;
	case Animation::Pulse:
	default:
		mode = 57 + rate_;
		break;
	}
	encoder_.setAnimation(mode);
}

// ------------------------------------------------------

void ofxParameterTwister::update() {

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

void pal::Kontrol::ofxParameterTwister::Encoder::setState(State s_, bool force_)
{
	if (s_ == mState && force_ == false) {
		return;
	}

	// ----------| invariant: state change requested, or forced

	switch (s_)
	{
	case pal::Kontrol::ofxParameterTwister::Encoder::State::DISABLED:
		// we need to switch off the status LED
		sendToSwitch(0);
		// we need to switch off the rotary status LED
		sendToRotary(0);
		setBrightnessRotary(0.f);
		setBrightnessRGB(1.f);
		break;
	case pal::Kontrol::ofxParameterTwister::Encoder::State::ROTARY:
		sendToSwitch(0);
		setBrightnessRotary(1.f);
		setBrightnessRGB(0.0f);
		break;
	case pal::Kontrol::ofxParameterTwister::Encoder::State::SWITCH:
		sendToRotary(0);
		setBrightnessRotary(0.f);
		setBrightnessRGB(1.0f);
		break;
	default:
		break;
	}

	mState = s_;
}

// ------------------------------------------------------

void pal::Kontrol::ofxParameterTwister::Encoder::setValue(uint8_t v_) {

	switch (mState)
	{
	case pal::Kontrol::ofxParameterTwister::Encoder::State::DISABLED:
		ofLogError() << "cannot send value to diabled encoder" << pos;
		break;
	case pal::Kontrol::ofxParameterTwister::Encoder::State::ROTARY:
		sendToRotary(v_);
		break;
	case pal::Kontrol::ofxParameterTwister::Encoder::State::SWITCH:
		sendToSwitch(v_);
		break;
	default:
		break;
	}

}

// ------------------------------------------------------

void pal::Kontrol::ofxParameterTwister::Encoder::sendToSwitch(uint8_t v_) {
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	vector<unsigned char> msg{
		0xB1,					// SWITCH listens on channel 1
		pos,					// device id
		v_,						// value
	};
	mMidiOut->sendMessage(&msg);

	ofLogVerbose() << ">>" << setw(2) << 1 * pos << " SWI " << " : " << setw(3) << v_ * 1;
}

// ------------------------------------------------------

void pal::Kontrol::ofxParameterTwister::Encoder::sendToRotary(uint8_t v_) {
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	vector<unsigned char> msg{
		0xB0,					// ROTARY listens on channel 0
		pos,					// device id
		v_,						// value
	};

	mMidiOut->sendMessage(&msg);

	ofLogVerbose() << ">>" << setw(2) << 1 * pos << " ROT " << " : " << setw(3) << v_ * 1;
}

// ------------------------------------------------------

void pal::Kontrol::ofxParameterTwister::Encoder::setHueRGB(float h_)
{
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	unsigned char val = std::roundf(ofMap(h_, 0.f, 1.f, 1, 126, true));

	vector<unsigned char> msg{
		0xB1,					// color control channel 1 
		pos,					// device id
		val,
	};

	mMidiOut->sendMessage(&msg);

}

// ------------------------------------------------------

void pal::Kontrol::ofxParameterTwister::Encoder::setBrightnessRGB(float b_)
{
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	unsigned char val = std::roundf(ofMap(b_, 0.f, 1.f, 17, 47, true));

	vector<unsigned char> msg{
		0xB2,					// animation control channel 2 
		pos,					// device id
		val,
	};

	mMidiOut->sendMessage(&msg);

}

// ------------------------------------------------------

void pal::Kontrol::ofxParameterTwister::Encoder::setBrightnessRotary(float b_)
{
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	unsigned char val = std::roundf(ofMap(b_, 0.f, 1.f, 65, 95, true));
	
	vector<unsigned char> msg{
		0xB2,					// animation control channel 2
		pos,					// device id
		val,
	};

	mMidiOut->sendMessage(&msg);

}

// ------------------------------------------------------

void pal::Kontrol::ofxParameterTwister::Encoder::setAnimation(uint8_t v_)
{
	if (mMidiOut == nullptr)
		return;

	// ----------| invariant: midiOut is not nullptr

	vector<unsigned char> msg{
		0xB2,					// animation control channel 2
		pos,					// device id
		v_,
	};

	mMidiOut->sendMessage(&msg);

}
