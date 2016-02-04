#include "ofxMidiFighter.h"
#include "RtMidi.h"

#include "ofParameter.h"


using namespace pal::Kontrol;

// ------------------------------------------------------

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

		ofLog() << ostr.str();

		tCh->send(std::move(msg));
	}
}

// ------------------------------------------------------

void MidiFighter::setup() {

	// this is where we try to establish a midi connection
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
					mMidiIn->setCallback(&_midi_callback, &mTch);

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

	// this is where we try to establish a midi connection
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
		std::cout << "MIDI input exception:" << std::endl;
		error.printMessage();
	}
}

// ------------------------------------------------------

void MidiParam<float>::valueChanged(float & value) {
	//ofLogNotice() << "value" << std::setw(10) << value << " parameter: " << mParam->getName();
}

void MidiParam<bool>::valueChanged(bool & value) {
	//ofLogNotice() << "BOOL value" << std::setw(10) << value << " parameter: " << mParam->getName();
}

vector<unsigned char> MidiParam<float>::getMessage() {
	vector<unsigned char> msg(3);
	msg.at(0) = mAddr >> 8;
	msg.at(1) = mAddr & 0x00FF;
	msg.at(2) = std::roundf(ofMap(*mParam, mParam->getMin(), mParam->getMax(), 0, 127));
	return msg;

};
vector<unsigned char> MidiParam<bool>::getMessage() {
	vector<unsigned char> msg(3);
	msg.at(0) = mAddr >> 8;
	msg.at(1) = mAddr & 0x00FF;
	msg.at(2) = (*mParam ? 127 : 0);
	return msg;
};

// ------------------------------------------------------

void MidiFighter::setParams(const ofParameterGroup& group_)
{
	// clean old midiparams, and 
	// remove all listeners, if there are any:

	for (const auto & m : mMidiParams) {
		// we need to use a raw pointer instead of our unique_ptr 
		// since a unique-ptr cannot be dynamic_cast.
		if (auto concreteListener = dynamic_cast<MidiParam<float>*>(m.second.get())) {
			ofLogNotice() << "removing float listener";
			concreteListener->mParam->removeListener(concreteListener, &MidiParam<float>::valueChanged);
		} else if (auto concreteListener = dynamic_cast<MidiParam<bool>*>(m.second.get())) {
			ofLogNotice() << "removing bool listener";
			concreteListener->mParam->removeListener(concreteListener, &MidiParam<bool>::valueChanged);
		}
	}

	// now we can clear the midiParams list.
	mMidiParams.clear();

	mParamGroup = group_;

	// now that we have our parameter group, we want to assign up to 16 parameters to 
	// individual midi controls.

	uint8_t i = 0;
	for (auto &abstractParam : mParamGroup) {
		// we should have a parameter reference in this now.

		// now let's map the parameter to a midi address.

		/*

		we're expecting our midi messges to arrive as CC messages.
		'CC' stands for continous controller.

		CC messages have the (most significant) first half-byte set to B,
		so anything xB... is a continous controller message.

		byte 0 .. controller message / channel number

		The least significant, or the second half-byte is the channel
		on which the signal comes in.

		CC messages have two parameter bytes:

		byte 1 .. controller number
		byte 2 .. controller value

		*/

		// so let's construct an address:

		uint16_t addr = 0xB000;
		addr += i;	// the least significant byte contains the device number: 0..127

		if (const auto &concreteParam = dynamic_pointer_cast<ofParameter<float>>(abstractParam)) {

			// bingo - we have a float parameter.
			addr += (0 << 8);		// if we have a float, use the knob, which is on channel 0

			ofLogNotice() << "0x" << std::hex << addr << ": param: " << setw(3) << (int)i << " : float";

			// add listener for when parameter changes
			// for this to work, we have to build an object around the responder
			// this is HORRIBLY hacky, but it's the only way we may keep track 
			// of the original parameter that triggered the action
			// since the way of keeping track of it is to register a different 
			// listener for each object.
			auto floatMidiParam = make_unique<MidiParam<float>>();
			floatMidiParam->mParam = concreteParam.get();
			floatMidiParam->mAddr = addr;

			auto midiMessage = floatMidiParam->getMessage();
			mMidiOut->sendMessage(&midiMessage);

			// now we set the listener to point to the valuechanged method,
			// after having set the this pointer to our oh so fancy listener object.
			// let's pray it never gets relocated.
			concreteParam->addListener(floatMidiParam.get(), &MidiParam<float>::valueChanged);
			mMidiParams[addr] = (std::move(floatMidiParam));

			// now, let's update the midi device with our current value

		} else if (auto concreteParam = dynamic_pointer_cast<ofParameter<bool>>(abstractParam)) {
			// bingo - we have a bool paramter.

			addr |= (1 << 8);		// if we have a bool, use button, which is on channel 1

			ofLogNotice() << "0x" << std::hex << addr << ": param: " << setw(3) << (int)i << " : bool ";

			auto boolMidiParam = make_unique<MidiParam<bool>>();
			boolMidiParam->mParam = concreteParam.get();
			boolMidiParam->mAddr = addr;

			auto midiMessage = boolMidiParam->getMessage();
			
			mMidiOut->sendMessage(&midiMessage);

			concreteParam->addListener(boolMidiParam.get(), &MidiParam<bool>::valueChanged);
			mMidiParams[addr] = (std::move(boolMidiParam));
		}
		++i;
		if (i > 15) {
			ofLogWarning() << "Cannot assign more than 16 midi parameters";
			break;
		}
	}
}

// ------------------------------------------------------

void MidiFighter::update() {

	// 1. ) accumulate all messages received until now through channel
	// 2. ) update linked parameters if message found for them.

	MidiCCMessage m;
	while (mTch.tryReceive(m)) {

		uint16_t address = m.command_channel << 8;
		address += m.controller;

		auto it = mMidiParams.find(address);
		if (it != mMidiParams.end()) {
			// first, we'll have to cast this parameter so that we 
			// can figure out what type it is referencing

			if (auto midiParam = dynamic_cast<MidiParam<float>*>(it->second.get())) {
				// bingo - we have a float parameter
				if (auto p = midiParam->mParam) {
					p->set(ofMap(m.value, 0, 127, p->getMin(), p->getMax()));
				}

			} else if (auto midiParam = dynamic_cast<MidiParam<bool>*>(it->second.get())) {
				// bingo - we have a bool parameter
				if (auto p = midiParam->mParam) {
					p->set((m.value > 63 ? true : false));
				}
			}
		}
	}

}
// ------------------------------------------------------

MidiFighter::MidiFighter() {
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