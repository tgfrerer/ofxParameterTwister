#include "ofxMidiFighter.h"

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

		ofLog() << ostr.str();

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
		std::cout << "MIDI input exception:" << std::endl;
		error.printMessage();
	}
}


// ------------------------------------------------------

void MidiFighter::setParams(const ofParameterGroup& group_)
{
	// this is where we store the paramters into our internal parameter group

	// first, clean up all mapped controllers.


}

// ------------------------------------------------------

void MidiFighter::update() {

	

}
// ------------------------------------------------------