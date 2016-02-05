#include "ofxMidiFighter.h"

#include "RtMidi.h"

#include "ofParameter.h"


// Wishlist
// TODO re-eastablish midi connection if lost
// TODO make sure to not crash when midi connection is lost


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

MidiFighter::~MidiFighter() {
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

void MidiFighter::setup() {

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


// ------------------------------------------------------

void MidiFighter::setParams(const ofParameterGroup& group_)
{
	ofLog() << "Updating mapping" << endl;
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

				e.mELParamChange = param->newListener([&e, pMin, pMax](float & v_) {
					e.setValue(ofMap(v_, pMin, pMax, 0, 127, true));
				});

				e.updateParameter = [=](uint8_t v_) {
					param->set(ofMap(v_, 0, 127, pMin, pMax, true));
				};

			} else if (auto param = dynamic_pointer_cast<ofParameter<bool>>(*it)) {
				// we have a bool parameter
				e.setState(Encoder::State::SWITCH);
				e.setValue(*param == true ? 127 : 0);

				e.mELParamChange = param->newListener([&e](bool& v_) {
					e.setValue(v_ == true ? 127 : 0);
				});

				e.updateParameter = [=](uint8_t v_) {
					param->set((v_ < 64) ? true: false);
				};

			} else {
				// we cannot match this parameter, unfortunately
				e.setState(Encoder::State::DISABLED);
				e.mELParamChange = ofEventListener(); // reset listener
			}
			++it;
		} else {
			// no more parameters to map.
			e.setState(Encoder::State::DISABLED, true);
		}
	}

}

// ------------------------------------------------------

void MidiFighter::update() {

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

void pal::Kontrol::MidiFighter::Encoder::setState(State s_, bool force_)
{
	if (s_ == mState && force_ == false) {
		return;
	}

	// ----------| invariant: state change requested, or forced

	switch (s_)
	{
	case pal::Kontrol::MidiFighter::Encoder::State::DISABLED:
		// we need to switch off the status LED
		sendToSwitch(0);
		// we need to switch off the rotary status LED
		sendToRotary(0);
		setBrightnessRotary(0.f);
		setBrightnessRGB(1.f);
		break;
	case pal::Kontrol::MidiFighter::Encoder::State::ROTARY:
		// we need to switch off the status LED
		sendToSwitch(0);
		setBrightnessRotary(1.f);
		setBrightnessRGB(0.0f);
		break;
	case pal::Kontrol::MidiFighter::Encoder::State::SWITCH:
		// we need to switch off the rotary status LED
		sendToRotary(0);
		setBrightnessRotary(0.0f);
		setBrightnessRGB(1.f);
		break;
	default:
		break;
	}

	mState = s_;
}

// ------------------------------------------------------

void pal::Kontrol::MidiFighter::Encoder::setValue(uint8_t v_) {

	switch (mState)
	{
	case pal::Kontrol::MidiFighter::Encoder::State::DISABLED:
		ofLogError() << "cannot send value to diabled encoder" << pos;
		break;
	case pal::Kontrol::MidiFighter::Encoder::State::ROTARY:
		sendToRotary(v_);
		break;
	case pal::Kontrol::MidiFighter::Encoder::State::SWITCH:
		sendToSwitch(v_);
		break;
	default:
		break;
	}

}

// ------------------------------------------------------

void pal::Kontrol::MidiFighter::Encoder::sendToSwitch(uint8_t v_) {
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

void pal::Kontrol::MidiFighter::Encoder::sendToRotary(uint8_t v_) {
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

void pal::Kontrol::MidiFighter::Encoder::setBrightnessRotary(float b_)
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

void pal::Kontrol::MidiFighter::Encoder::setBrightnessRGB(float b_)
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
