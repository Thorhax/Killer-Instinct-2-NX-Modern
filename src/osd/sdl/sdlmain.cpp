// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont
//============================================================
//
//  sdlmain.cpp - main file for SDLMAME.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// OSD headers
#include "osdsdl.h"
#include "modules/lib/osdlib.h"
#include "modules/diagnostics/diagnostics_module.h"

// MAME headers
#include "emu.h"
#include "emuopts.h"
#include "main.h"
#include "video.h"

#include "corestr.h"

#include "osdepend.h"
#include "strconv.h"

#include <SDL2/SDL.h>

// only for oslog callback
#include <functional>

#ifdef SDLMAME_UNIX
#if (!defined(SDLMAME_MACOSX)) && (!defined(SDLMAME_EMSCRIPTEN)) && (!defined(SDLMAME_ANDROID)) && (!defined(__SWITCH__))
#ifndef SDLMAME_HAIKU
#include <fontconfig/fontconfig.h>
#endif
#endif
#ifdef SDLMAME_MACOSX
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#include <Carbon/Carbon.h>
#endif
#endif

// standard includes
#if !defined(SDLMAME_WIN32)
#include <unistd.h>
#endif


//============================================================
//  Global variables
//============================================================

#if defined(SDLMAME_UNIX) || defined(SDLMAME_WIN32)
int sdl_entered_debugger;
#endif


//============================================================
//  main
//============================================================

// we do some special sauce on Win32...

#if defined(SDLMAME_WIN32)
/* gee */
extern "C" DECLSPEC void SDLCALL SDL_SetModuleHandle(void *hInst);
#endif

#if defined(__SWITCH__)
#include <switch.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" int fileno(FILE *);
extern "C" int setenv(const char *name, const char *value, int overwrite);

extern "C" void userAppInit(void)
{
	romfsInit();
	mkdir("sdmc:/switch", 0777);
	mkdir("sdmc:/switch/kinst", 0777);
	mkdir("sdmc:/switch/kinst/roms", 0777);
	mkdir("sdmc:/switch/kinst/roms/kinst", 0777);
	mkdir("sdmc:/switch/kinst/ini", 0777);
	mkdir("sdmc:/switch/kinst/cfg", 0777);
	chdir("sdmc:/switch/kinst");

	setenv("HOME", "sdmc:/switch/kinst", 1);
	osd_setenv("HOME", "sdmc:/switch/kinst", 1);

	// Redirect stdout to persistent log file on SD card
	FILE *fout = freopen("sdmc:/switch/kinst/kinst.log", "w", stdout);
	if (fout)
	{
		setvbuf(stdout, nullptr, _IOLBF, 4096);
		// Mirror stderr to stdout so all MAME errors and exceptions appear in kinst.log
		dup2(fileno(stdout), STDERR_FILENO);
		setvbuf(stderr, nullptr, _IOLBF, 4096);
	}

	// Set preferred core 0 with full 3-core affinity mask so worker/audio threads can spawn onto Cores 1 & 2
	svcSetThreadCoreMask(CUR_THREAD_HANDLE, 0, (1 << 0) | (1 << 1) | (1 << 2));

	// Also attempt nxlink if launched via netloader
	if (R_SUCCEEDED(socketInitializeDefault()))
	{
		nxlinkStdio();
	}

	printf("=== MAME-NX Killer Instinct Log Started ===\n");
	fflush(stdout);
}

extern "C" void userAppExit(void)
{
	printf("=== MAME-NX Killer Instinct Exiting ===\n");
	fflush(stdout);
	fflush(stderr);
	socketExit();
	romfsExit();
}
#endif

int main(int argc, char** argv)
{
	std::vector<std::string> args = osd_get_command_line(argc, argv);
	int res = 0;

#if !defined(__SWITCH__)
	// disable I/O buffering
	setvbuf(stdout, (char *) nullptr, _IONBF, 0);
	setvbuf(stderr, (char *) nullptr, _IONBF, 0);
#endif

	// Initialize crash diagnostics
	diagnostics_module::get_instance()->init_crash_diagnostics();

#if defined(SDLMAME_ANDROID)
	/* Enable standard application logging */
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE);
#endif

	// FIXME: this should be done differently

#ifdef SDLMAME_UNIX
	sdl_entered_debugger = 0;
#if (!defined(SDLMAME_MACOSX)) && (!defined(SDLMAME_HAIKU)) && (!defined(SDLMAME_EMSCRIPTEN)) && (!defined(SDLMAME_ANDROID)) && (!defined(__SWITCH__))
	FcInit();
#endif
#endif

#if defined(__SWITCH__)
	// Allow main thread and all spawned threads to schedule across all 3 CPU cores (preferred core 0)
	svcSetThreadCoreMask(CUR_THREAD_HANDLE, 0, (1 << 0) | (1 << 1) | (1 << 2));
	SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG,
		"000038f853776974636820436f6e7400,Switch Controller,a:b1,b:b0,back:b11,dpdown:b15,dpleft:b12,dpright:b14,dpup:b13,leftshoulder:b6,leftstick:b4,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b7,rightstick:b5,righttrigger:b9,rightx:a2,righty:a3,start:b10,x:b3,y:b2,");

	if (args.size() <= 1)
	{
		args.push_back("kinst");
	}
	// Switch performance defaults
	bool has_video = false;
	bool has_autoframeskip = false;
	bool has_sleep = false;
	bool has_skip_gameinfo = false;
	bool has_numprocessors = false;
	bool has_audio_latency = false;
	bool has_joystickprovider = false;
	bool has_samplerate = false;
	for (const auto &arg : args)
	{
		if (arg == "-video") has_video = true;
		if (arg == "-autoframeskip" || arg == "-afs") has_autoframeskip = true;
		if (arg == "-sleep" || arg == "-nosleep") has_sleep = true;
		if (arg == "-skip_gameinfo") has_skip_gameinfo = true;
		if (arg == "-numprocessors" || arg == "-np") has_numprocessors = true;
		if (arg == "-audio_latency") has_audio_latency = true;
		if (arg == "-joystickprovider") has_joystickprovider = true;
		if (arg == "-samplerate" || arg == "-sr") has_samplerate = true;
	}
	if (!has_video)
	{
		args.push_back("-video");
		args.push_back("accel");
	}
	if (!has_autoframeskip)
	{
		args.push_back("-autoframeskip");
	}
	if (!has_sleep)
	{
		args.push_back("-nosleep");
	}
	if (!has_numprocessors)
	{
		args.push_back("-numprocessors");
		args.push_back("3");
	}
	if (!has_audio_latency)
	{
		args.push_back("-audio_latency");
		args.push_back("2");
	}
	if (!has_joystickprovider)
	{
		args.push_back("-joystickprovider");
		args.push_back("switch");
	}
	if (!has_skip_gameinfo)
	{
		args.push_back("-skip_gameinfo");
	}
	if (!has_samplerate)
	{
		args.push_back("-samplerate");
		args.push_back("48000");
	}
	args.push_back("-verbose");
#endif

	try
	{
		sdl_options options;
		sdl_osd_interface osd(options);
		osd.register_options();
		res = emulator_info::start_frontend(options, osd, args);
		printf("=== MAME frontend returned: %d ===\n", res);
	}
	catch (const std::exception &e)
	{
		printf("=== MAME FATAL EXCEPTION: %s ===\n", e.what());
		res = -1;
	}
	catch (...)
	{
		printf("=== MAME UNKNOWN EXCEPTION CAUGHT ===\n");
		res = -1;
	}
	fflush(stdout);
	fflush(stderr);

#ifdef SDLMAME_UNIX
#if (!defined(SDLMAME_MACOSX)) && (!defined(SDLMAME_HAIKU)) && (!defined(SDLMAME_EMSCRIPTEN)) && (!defined(SDLMAME_ANDROID)) && (!defined(__SWITCH__))
	if (!sdl_entered_debugger)
	{
		FcFini();
	}
#endif
#endif

	return res;
}
