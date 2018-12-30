
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "RtMidi.h"

RtMidiIn  *midiin = 0;
RtMidiOut *midiout = 0;

int inport = 0;
int outport = 0;

void selectDevices()
{
	printf(
		"DM Lite midi fixer version 0.1 (20181230)\n"
		"\n"
		"This program re-maps the midi commands coming in from the\n"
		"DM Lite drum set to something VSTs expect (more)\n"
		"\n"
		"You should have a midi loopback driver (such as LoopMidi)\n"
		"running before running this one. Oh, and if we get a crash,\n"
		"you may want to run this BEFORE running your DAW.\n"
		"\n"
		"To reiterate:\n"
		"1. run midi loopback driver\n"
		"2. run this one\n"
		"3. run your DAW\n"
		"\n"
		"Finally, use at your own risk. Okay?\n\n");
	
	// RtMidiIn constructor
	try {
		midiin = new RtMidiIn();
	}
	catch (RtMidiError &error) {
		error.printMessage();
		exit(EXIT_FAILURE);
	}
	// Check inputs.
	unsigned int nPorts = midiin->getPortCount();
	printf("There are %d MIDI inputs available: (Choose the DMLite device port)\n", nPorts);
	std::string portName;
	for (unsigned int i = 0; i<nPorts; i++) {
		try {
			portName = midiin->getPortName(i);
		}
		catch (RtMidiError &error) {
			error.printMessage();
			delete midiin;
			delete midiout;
			exit(EXIT_FAILURE);
		}
		printf("%d) %s\n", i, portName.c_str());
	}
	printf("q) Cancel\n");

	inport = -1;
	do
	{
		inport = _getch() - '0';
		if (inport == 'q' - '0')
		{
			delete midiin;
			delete midiout;
			exit(EXIT_FAILURE);
		}
	} 
	while (inport < 0 || inport >= (signed)nPorts);

	// RtMidiOut constructor
	try {
		midiout = new RtMidiOut();
	}
	catch (RtMidiError &error) {
		error.printMessage();
		exit(EXIT_FAILURE);
	}
	// Check outputs.
	nPorts = midiout->getPortCount();
	printf("There are %d MIDI outputs available: (Choose the loopback port)\n", nPorts);
	for (unsigned int i = 0; i<nPorts; i++) {
		try {
			portName = midiout->getPortName(i);
		}
		catch (RtMidiError &error) {
			error.printMessage();
			delete midiin;
			delete midiout;
			exit(EXIT_FAILURE);
		}
		printf("%d) %s\n", i, portName.c_str());
	}
	printf("q) Cancel\n");
	outport = -1;
	do
	{
		outport = _getch() - '0';
		if (outport == 'q' - '0')
		{
			delete midiin;
			delete midiout;
			exit(EXIT_FAILURE);
		}
	} while (outport < 0 || outport >= (signed)nPorts);
	printf("\nStart your DAW now and use the loopback port as midi input. Press any key in this window to quit.\n");
}

void mycallback(double deltatime, std::vector< unsigned char > *message, void *userData)
{
	printf("%7.5f :", deltatime);
	int i;
	for (i = 0; i < (signed)message->size(); i++)
		printf(" %02x", (*message)[i]);	
	if (((*message)[0] & 0xf0) == 0x90 && (*message)[2] > 0)
	{
		printf("\n");
		// Tweak the note on messages
		(*message)[0] = 0x90; // Change channel to 0 from 9
		//(*message)[1] += 0x3c - 0x24; // Tweak note offset (needs more work)
		// 24 2e 31 33 26 30 2d 2b
		switch ((*message)[1])
		{
		case 0x24: (*message)[1] = 0x3c; break;
		case 0x2e: (*message)[1] = 0x3e; break;
		case 0x31: (*message)[1] = 0x40; break;
		case 0x33: (*message)[1] = 0x41; break;
		case 0x26: (*message)[1] = 0x43; break;
		case 0x30: (*message)[1] = 0x45; break;
		case 0x2d: (*message)[1] = 0x47; break;
		case 0x2b: (*message)[1] = 0x48; break;
		}
		midiout->sendMessage(message);
		printf("%7.5f :", deltatime);
		for (i = 0; i < (signed)message->size(); i++)
			printf(" %02x", (*message)[i]);
		printf(" sent\n");
		// Ignore other messages
	}
	else
	{
		printf(" ignored\n");
	}
}

void mainLoop()
{
	midiout->openPort(outport);
	midiin->openPort(inport);
	midiin->setCallback(mycallback);
	midiin->ignoreTypes(false, true, false);
	_getch();
}

int main()
{
	selectDevices();
	mainLoop();
	delete midiin;
	delete midiout;
	return 0;
}
