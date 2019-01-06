#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "RtMidi.h"

RtMidiIn  *drumin = 0;
RtMidiOut *midiout = 0;

int inport = 0;
int outport = 0;

Uint32 livetick = 0;

GLuint background_tex;

struct Preset
{
	char * name;
	int map[8];
};

#define MAX_PRESET 100

Preset preset[MAX_PRESET];
int presets = 0;
char * presetnames;

void loadpresets()
{
	FILE * f; fopen_s(&f, "dmlitefix_presets.txt", "r");
	if (!f)
	{
		presetnames = _strdup("No presets found\0\0");
		return;
	}
	char line[512];

	while (!feof(f))
	{
		int i = 0;
		int c = 0;
		int data = 0;
		while (!feof(f) && c != '\n')
		{
			c = fgetc(f);
			if (!(c == ' ' || c == '\t' || c == '\n')) data = 1;
			if (data)
			{
				line[i] = c;
				i++;
			}
		}
		while (i > 0 && (line[i] == ' ' || line[i] == '\t' || line[i] == '\n')) i--;
		i++;
		line[i] = 0;
		if (line[0] == '\"')
		{
			i = 1;
			while (line[i] != '\"' && line[i] != 0) i++;
			
			preset[presets].name = new char[i];
			memcpy(preset[presets].name, line + 1, i - 1);
			preset[presets].name[i - 1] = 0;

			i++;
			while (line[i] != 0 && line[i] != ',') i++;
			if (line[i]) i++;
			int m = 0;
			char t[100];
			int j = 0;
			while (!(line[i] == 0 && j == 0) && m < 8)
			{
				if (line[i] == ',' || line[i] == 0)
				{
					t[j] = 0;
					char * ptr;
					preset[presets].map[m] = strtol(t, &ptr, 0);
					j = 0;
					m++;
					i++;
				}
				else
				if (line[i] == ' ' || line[i] == '\t') i++;
				else
				{
					t[j] = line[i];
					j++;
					i++;
				}
			}

			presets++;
		}
	}

	int i;
	int len = strlen("Select preset") + 1;
	for (i = 0; i < presets; i++)
		len += strlen(preset[i].name) + 1;
	presetnames = new char[len + 2];
	memset(presetnames, 0, len + 2);
	len = 0;
	memcpy(presetnames, "Select preset", strlen("Select preset"));
	len += strlen("Select preset") + 1;
	for (i = 0; i < presets; i++)
	{
		int l = strlen(preset[i].name);
		memcpy(presetnames + len, preset[i].name, l);
		len += l + 1;
	}
	fclose(f);
}

unsigned int loadtexture(const char * aFilename)
{
	int x, y, comp;
	unsigned char *image = stbi_load(aFilename, &x, &y, &comp, 4);
	if (!image)
		return 0;
	unsigned int tex;
	
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	stbi_image_free(image);
	return tex;
}

const char *midinotes =
"C-1\0C#-1\0D-1\0D#-1\0E-1\0F-1\0F#-1\0G-1\0G#-1\0A-1\0A#-1\0B-1\0"
"C0\0C#0\0D0\0D#0\0E0\0F0\0F#0\0G0\0G#0\0A0\0A#0\0B0\0"
"C1\0C#1\0D1\0D#1\0E1\0F1\0F#1\0G1\0G#1\0A1\0A#1\0B1\0"
"C2\0C#2\0D2\0D#2\0E2\0F2\0F#2\0G2\0G#2\0A2\0A#2\0B2\0"
"C3\0C#3\0D3\0D#3\0E3\0F3\0F#3\0G3\0G#3\0A3\0A#3\0B3\0"
"C4\0C#4\0D4\0D#4\0E4\0F4\0F#4\0G4\0G#4\0A4\0A#4\0B4\0"
"C5\0C#5\0D5\0D#5\0E5\0F5\0F#5\0G5\0G#5\0A5\0A#5\0B5\0"
"C6\0C#6\0D6\0D#6\0E6\0F6\0F#6\0G6\0G#6\0A6\0A#6\0B6\0"
"C7\0C#7\0D7\0D#7\0E7\0F7\0F#7\0G7\0G#7\0A7\0A#7\0B7\0"
"C8\0C#8\0D8\0D#8\0E8\0F8\0F#8\0G8\0G#8\0A8\0A#8\0B8\0"
"C9\0C#9\0D9\0D#9\0E9\0F9\0F#9\0G9\0\0";

int notemap1 = 0x3c;
int notemap2 = 0x3e;
int notemap3 = 0x40;
int notemap4 = 0x41;
int notemap5 = 0x43;
int notemap6 = 0x45;
int notemap7 = 0x47;
int notemap8 = 0x48;

int ramp = 1;

void mycallback(double deltatime, std::vector< unsigned char > *message, void *userData)
{
	if (((*message)[0] & 0xf0) == 0x90)
	{
		// Change channel to 0 from 9, as that's what VSTs seem to expect
		(*message)[0] = 0x90;

		// Tweak the note on messages
		switch ((*message)[1])
		{
		case 0x24: (*message)[1] = notemap1; break;
		case 0x2e: (*message)[1] = notemap2; break;
		case 0x31: (*message)[1] = notemap3; break;
		case 0x33: (*message)[1] = notemap4; break;
		case 0x26: (*message)[1] = notemap5; break;
		case 0x30: (*message)[1] = notemap6; break;
		case 0x2d: (*message)[1] = notemap7; break;
		case 0x2b: (*message)[1] = notemap8; break;
		}

		int vol = (*message)[2];
		float volf = vol / (float)0x7f;

		if (ramp == 0) { volf *= volf; vol = (int)floor(volf * 0x7f); }
		if (ramp == 2) { volf = 1 - volf;  volf *= volf; volf = 1 - volf; vol = (int)floor(volf * 0x7f); }
		if (ramp == 3) { vol = 0x7f; }

		(*message)[2] = vol;
		midiout->sendMessage(message);
	}
	else
	if ((*message)[0] == 0xf8)
	{
		// timing message
		livetick = SDL_GetTicks();
	}
}


int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window* window = SDL_CreateWindow("DM Lite Fix", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 420, SDL_WINDOW_OPENGL);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

	drumin = new RtMidiIn();
	midiout = new RtMidiOut();

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	background_tex = loadtexture("dmlite.png");
	loadpresets();
	bool done = false;
	bool rescan = true;
	bool quit = false;
	int fail = 0;
	char * drumportnames = 0;
	char * outportnames = 0;
	// Setup loop
	while (!done)
	{
		if (rescan)
		{
			delete[] drumportnames;
			delete[] outportnames;
			int ports = drumin->getPortCount();
			int len = 0;
			int i;
			for (i = 0; i < ports; i++)
				len += drumin->getPortName(i).length();
			drumportnames = new char[len + ports + 2];
			memset(drumportnames, 0, len + ports + 2);
			len = 0;
			for (i = 0; i < ports; i++)
			{
				std::string portname = drumin->getPortName(i);
				memcpy(drumportnames + len, portname.c_str(), portname.length());
				len += portname.length() + 1;
				if (portname.find("Alesis DM Lite") != portname.npos) inport = i;
			}

			len = 0;
			ports = midiout->getPortCount();
			for (i = 0; i < ports; i++)
				len += midiout->getPortName(i).length();
			outportnames = new char[len + ports + 2];
			memset(outportnames, 0, len + ports + 2);
			len = 0;
			for (i = 0; i < ports; i++)
			{
				std::string portname = midiout->getPortName(i);
				memcpy(outportnames + len, portname.c_str(), portname.length());
				len += portname.length() + 1;
				if (portname.find("loopMIDI") != portname.npos) outport = i;
				if (portname.find("loopBe") != portname.npos) outport = i;
				if (portname.find("Virtual MIDI") != portname.npos) outport = i;
			}

			rescan = false;
		}

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
			{
				done = true;
				quit = true;
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		ImGui::SetNextWindowBgAlpha(0);
		ImGui::SetNextWindowSize(ImVec2(600, 420));
		ImGui::SetNextWindowPos(ImVec2(0, 0));

		ImGui::Begin("main", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);

		ImGui::SetCursorPos(ImVec2(100, 10));
		if (fail == 0)
			ImGui::Text(
				"DM Lite midi fixer version 0.5 GUI (20190106)\n"
				"by Jari Komppa http://iki.fi/sol/\n"
				"\n"
				"\n"
				"This program re-maps the midi commands coming in from the\n"
				"DM Lite drum set to something VSTs expect (more or less)\n"
				"\n"
				"You should have a midi loopback driver (such as LoopMidi)\n"
				"running before running this one. Oh, and sometimes DAWs\n"
				"block us from opening ports, so you may want to run this\n"
				"BEFORE running your DAW.\n"
				"\n"
				"So MIDI commands flow like this:\n"
				"DM Lite -> this program -> loopback -> DAW\n"
				"\n"
				"Finally, use at your own risk. Okay?\n");
		else
			ImGui::Text(
				"\n\n\n\n\n\n"
				"Failed to open input and/or output port.\n"
				"\n"
				"This typically happens if your DAW has opened\n"
				"all of the MIDI ports. Try quiting your DAW\n"
				"and trying again.");


		ImGui::SetCursorPos(ImVec2(100, 250));
		ImGui::Combo("Input port", &inport, drumportnames);
		ImGui::SetCursorPos(ImVec2(100, 280));
		ImGui::Combo("Output port", &outport, outportnames);
		ImGui::SetCursorPos(ImVec2(100, 310));
		if (ImGui::Button("Start", ImVec2(390, 70)))
		{
			fail = 0;
			try {
				midiout->openPort(outport);
				drumin->openPort(inport);
			}
			catch (const std::exception& e)
			{
			}

			if (!midiout->isPortOpen())
			{
				fail = 1;
			}

			if (!drumin->isPortOpen())
			{
				fail = 1;
			}

			if (fail)
			{
				if (midiout->isPortOpen()) midiout->closePort();
				if (drumin->isPortOpen()) drumin->closePort();
			}
			else
			{
				done = true;
			}
		}

		ImGui::SetCursorPos(ImVec2(100, 390));
		if (ImGui::Button("Re-scan devices", ImVec2(190,20))) rescan = true;
		ImGui::SameLine();
		if (ImGui::Button("Quit", ImVec2(190, 20))) { done = true; quit = true; }
		ImGui::End();

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		//DemoTexQuad(background_tex, -1, -1, 1, 1);
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}
	delete[] drumportnames;
	delete[] outportnames;

	drumin->setCallback(mycallback);
	drumin->ignoreTypes(false, false, false);

    // Main loop
    done = false;
    while (!done && !quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();


		ImGui::SetNextWindowBgAlpha(0);
		ImGui::SetNextWindowSize(ImVec2(600, 420));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
			
        ImGui::Begin("main",0, ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove| ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoCollapse| ImGuiWindowFlags_NoBackground| ImGuiWindowFlags_NoSavedSettings);

		ImGui::SetCursorPos(ImVec2(0, 0));
		ImGui::Image((void*)(intptr_t)background_tex, ImVec2(600, 420));

		ImGui::PushItemWidth(100);
		ImGui::SetCursorPos(ImVec2(5, 0));
		ImGui::Combo("Volume ramp", &ramp, "Quiet\0Default\0Loud\0Max\0\0");
		ImGui::PopItemWidth();

		ImGui::SetCursorPos(ImVec2(500, 4));
		if (SDL_GetTicks() - livetick < 50)
			ImGui::Text("    Connected");
		else
			if (SDL_GetTicks() % 1000 < 800)
				ImGui::Text("Disconnected!");

		ImGui::PushItemWidth(60);

		ImGui::SetCursorPos(ImVec2(100, 80));			
		ImGui::Combo("##nm2", &notemap2, midinotes);

		ImGui::SetCursorPos(ImVec2(200, 50));
		ImGui::Combo("##nm3", &notemap3, midinotes);

		ImGui::SetCursorPos(ImVec2(380, 50));
		ImGui::Combo("##nm4", &notemap4, midinotes);

		ImGui::SetCursorPos(ImVec2(160, 165));
		ImGui::Combo("##nm5", &notemap5, midinotes);

		ImGui::SetCursorPos(ImVec2(245, 135));
		ImGui::Combo("##nm6", &notemap6, midinotes);

		ImGui::SetCursorPos(ImVec2(345, 135));
		ImGui::Combo("##nm7", &notemap7, midinotes);

		ImGui::SetCursorPos(ImVec2(450, 170));
		ImGui::Combo("##nm8", &notemap8, midinotes);

		ImGui::SetCursorPos(ImVec2(280, 320));
		ImGui::Combo("##nm1", &notemap1, midinotes);

		ImGui::PopItemWidth();

		ImGui::SetCursorPos(ImVec2(100, 400));
		int presetno = 0;
		if (ImGui::Combo("##ps", &presetno, presetnames))
		{ 
			notemap1 = preset[presetno - 1].map[0];
			notemap2 = preset[presetno - 1].map[1];
			notemap3 = preset[presetno - 1].map[2];
			notemap4 = preset[presetno - 1].map[3];
			notemap5 = preset[presetno - 1].map[4];
			notemap6 = preset[presetno - 1].map[5];
			notemap7 = preset[presetno - 1].map[6];
			notemap8 = preset[presetno - 1].map[7];
		}

        ImGui::End();


        // Rendering
        ImGui::Render();		
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
		//DemoTexQuad(background_tex, -1, -1, 1, 1);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

	delete midiout;
	delete drumin;

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
