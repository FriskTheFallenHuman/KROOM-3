/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

/*
===============================================================================

	Definitions for information that is related to a licensee's game name and location.

===============================================================================
*/

#define GAME_NAME						"DOOM 3"		/// appears in errors
#define GAME_NAME_LOWER					"kroom3"		// appears in screenshot names

#define ENGINE_VERSION					"KROOM 3 0.5.0"	// printed in console, used for window title

#ifdef ID_REPRODUCIBLE_BUILD
	// for reproducible builds we hardcode values that would otherwise come from __DATE__ and __TIME__
	// NOTE: remember to update esp. the date for (pre-) releases and RCs and the like
	#define ID__DATE__  "Sept 16 2024"
	#define ID__TIME__  "14:48:00"

#else // not reproducible build, use __DATE__ and __TIME__ macros
	#define ID__DATE__  __DATE__
	#define ID__TIME__  __TIME__
#endif

// paths
#define	BASE_GAMEDIR					"base"

// filenames
#ifndef CONFIG_FILE
	#define CONFIG_FILE						"Kroom3Config.cfg"
#endif

// base folder where the source code lives
#define SOURCE_CODE_BASE_FOLDER			"neo"


// default idnet host address
#ifndef IDNET_HOST
	#define IDNET_HOST					"idnet.ua-corp.com"
#endif

// default idnet master port
#ifndef IDNET_MASTER_PORT
	#define IDNET_MASTER_PORT			"27650"
#endif

// default network server port
#ifndef PORT_SERVER
	#define	PORT_SERVER					27666
#endif

// broadcast scan this many ports after PORT_SERVER so a single machine can run multiple servers
#define	NUM_SERVER_PORTS				4

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
#define ASYNC_PROTOCOL_MAJOR			1

// Savegame Version
// Update when you can no longer maintain compatibility with previous savegames
// NOTE: a seperate core savegame version and game savegame version could be useful
// 16: Doom v1.1
// 17: Doom v1.2 / D3XP. Can still read old v16 with defaults for new data
// 18: KROOM3
#define SAVEGAME_VERSION				18

// <= Doom v1.1: 1. no DS_VERSION token ( default )
// Doom v1.2: 2
// KROOM3 : 3
#define RENDERDEMO_VERSION				3

// editor info
#define EDITOR_REGISTRY_KEY				"IdStudio"
#define EDITOR_WINDOWTEXT				"IdStudio"

// win32 info
#define WIN32_CONSOLE_CLASS				"DOOM3_WinConsole"
#define	WIN32_WINDOW_CLASS_NAME			"DOOM3"
#define	WIN32_FAKE_WINDOW_CLASS_NAME	"DOOM3_WGL_FAKE"

// Linux info
#ifndef LINUX_DEFAULT_PATH // allow overriding it from the build system with -DLINUX_DEFAULT_PATH="/bla/foo/whatever"
	#define LINUX_DEFAULT_PATH				"/usr/local/games/doom3"
#endif

// CD Key file info
// goes into BASE_GAMEDIR whatever the fs_game is set to
// two distinct files for easier win32 installer job
#define CDKEY_FILE						"doomkey"
#define XPKEY_FILE						"xpkey"
#define CDKEY_TEXT						"\n// Do not give this file to ANYONE.\n" \
										"// id Software or Zenimax will NEVER ask you to send this file to them.\n"