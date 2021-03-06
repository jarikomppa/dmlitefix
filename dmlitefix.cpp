#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "RtMidi.h"

RtMidiIn  *drumin = 0;
RtMidiOut *midiout = 0;

int inport = 0;
int outport = 0;
int starttick = 0;

void selectDevices()
{
	printf(
		"DM Lite midi fixer version 0.2 (20190105)\n"
		"\n"
		"This program re-maps the midi commands coming in from the\n"
		"DM Lite drum set to something VSTs expect (more or less)\n"
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
		"Finally, use at your own risk. Okay?\n");
	
	// RtMidiIn constructor
	try
	{
		drumin = new RtMidiIn();
	}
	catch (RtMidiError &error) 
	{
		error.printMessage();
		exit(EXIT_FAILURE);
	}
	
	int pref = 0;
	int found = 0;
	// Check inputs.
	unsigned int nPorts = drumin->getPortCount();
	printf("\nThere are %d MIDI inputs available: (Choose the DMLite device port)\n", nPorts);
	std::string portName;
	for (unsigned int i = 0; i<nPorts; i++) 
	{
		try 
		{
			portName = drumin->getPortName(i);
		}
		catch (RtMidiError &error) 
		{
			error.printMessage();
			delete drumin;
			delete midiout;
			exit(EXIT_FAILURE);
		}
		printf("%d) %s", i, portName.c_str());
		if (!found && portName.find("Alesis DM Lite") != portName.npos) { printf(" << default"); found = 1; pref = i; }
		printf("\n");
	}
	printf("Space: %d\n", pref);
	printf("q) Cancel\n");

	inport = -1;
	do
	{
		inport = _getch() - '0';
		if (inport == ' ' - '0') inport = pref;
		if (inport == 'q' - '0')
		{
			delete drumin;
			delete midiout;
			exit(EXIT_FAILURE);
		}
	} 
	while (inport < 0 || inport >= (signed)nPorts);

	pref = 0;
	found = 0;
	// RtMidiOut constructor
	try 
	{
		midiout = new RtMidiOut();
	}
	catch (RtMidiError &error) 
	{
		error.printMessage();
		exit(EXIT_FAILURE);
	}
	// Check outputs.
	nPorts = midiout->getPortCount();
	printf("\nThere are %d MIDI outputs available: (Choose the loopback port)\n", nPorts);
	for (unsigned int i = 0; i<nPorts; i++) 
	{
		try 
		{
			portName = midiout->getPortName(i);
		}
		catch (RtMidiError &error) 
		{
			error.printMessage();
			delete drumin;
			delete midiout;
			exit(EXIT_FAILURE);
		}
		printf("%d) %s", i, portName.c_str());
		if (!found && portName.find("Alesis DM Lite") != portName.npos) pref++;
		if (!found && portName.find("Microsoft GS Wavetable Synth") != portName.npos) pref++;
		if (!found && portName.find("loopMIDI") != portName.npos) { found = 1; pref = i; printf(" << default"); }
		printf("\n");
	}
	printf("Space: %d\n", pref);
	printf("q) Cancel\n");
	outport = -1;
	do
	{
		outport = _getch() - '0';
		if (outport == ' ' - '0') outport = pref;
		if (outport == 'q' - '0')
		{
			delete drumin;
			delete midiout;
			exit(EXIT_FAILURE);
		}
	} while (outport < 0 || outport >= (signed)nPorts);
	printf("\n\nStart your DAW now and use the loopback port as midi input. Press any key in this window to quit.\n\n");
}


void mycallback(double deltatime, std::vector< unsigned char > *message, void *userData)
{
	printf("\r                                        \r%7.3f:", deltatime);

	int i;
	for (i = 0; i < (signed)message->size(); i++)
		printf(" %02x", (*message)[i]);	

	if (((*message)[0] & 0xf0) == 0x90)
	{
		printf(" received.\n");
		// Change channel to 0 from 9, as that's what VSTs seem to expect
		(*message)[0] = 0x90; 

		// Tweak the note on messages
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
		printf("%7.3f:", deltatime);
		for (i = 0; i < (signed)message->size(); i++)
			printf(" %02x", (*message)[i]);
		printf(" sent.\n");
	}
	else
	{
		if ((*message)[0] == 0xf8)
		{
			// timing message
			printf(" timing. Uptime: %d\r", (GetTickCount() - starttick) / 1000);
		}
		else
		{
			// Ignore other messages
			printf(" ignored.\n");
		}
	}
}

void mainLoop()
{
	midiout->openPort(outport);
	drumin->openPort(inport);

	if (!midiout->isPortOpen())
	{
		printf("Unable to open output port. Close your DAW and try again.\n");
		delete drumin;
		delete midiout;
		exit(EXIT_FAILURE);
	}

	if (!drumin->isPortOpen())
	{
		printf("Unable to open input port. Close your DAW and try again.\n");
		delete drumin;
		delete midiout;
		exit(EXIT_FAILURE);
	}

	drumin->setCallback(mycallback);
	drumin->ignoreTypes(false, true, false);
	//drumin->ignoreTypes(false, false, false); // for testing: shows timing messages
	_getch();
}

int main()
{
	selectDevices();
	starttick = GetTickCount();
	mainLoop();
	delete drumin;
	delete midiout;
	return 0;
}
