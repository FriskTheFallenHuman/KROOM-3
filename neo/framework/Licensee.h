/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
===============================================================================

	Definitions for information that is related to a licensee's game name and location.

===============================================================================
*/

#define GAME_NAME						"KROOM 3: BFG"		// appears on window titles and errors
#define SAVE_PATH						"\\id Software\\KROOM 3 BFG"
#define ENGINE_VERSION					"KROOM 3: BFG"	// printed in console, used for window title

#ifdef ID_REPRODUCIBLE_BUILD
	// for reproducible builds we hardcode values that would otherwise come from __DATE__ and __TIME__
	// NOTE: remember to update esp. the date for (pre-) releases and RCs and the like
	#define ID__DATE__  "Aug 15 2024"
	#define ID__TIME__  "13:37:42"

#else // not reproducible build, use __DATE__ and __TIME__ macros
	#define ID__DATE__  __DATE__
	#define ID__TIME__  __TIME__
#endif

// paths
#define	BASE_GAMEDIR					"base"

// filenames
#ifndef CONFIG_FILE
	#define CONFIG_FILE					"kroom3.cfg"
#endif

// default network server port
#ifndef PORT_SERVER
	#define	PORT_SERVER					"27015"
#endif

// <= Doom v1.1: 1. no DS_VERSION token ( default )
// Doom v1.2:  2
// Doom 3 BFG: 3
#define RENDERDEMO_VERSION				3

// editor info
#define EDITOR_DEFAULT_PROJECT			"doom.qe4"
#define EDITOR_REGISTRY_KEY				"DOOMRadiant"
#define EDITOR_WINDOWTEXT				"DOOMEdit"

// win32 info
#define WIN32_CONSOLE_CLASS				"D3BFG_WinConsole"
#define	WIN32_WINDOW_CLASS_NAME			"D3BFG"
#define	WIN32_FAKE_WINDOW_CLASS_NAME	"D3BFG_WGL_FAKE"

// RB begin
// Default base path (used only if none could be found)
#ifdef __APPLE__
	#define DEFAULT_BASEPATH				"/Applications/RBDoom3BFG.app/Contents/Resources"
#else
	#define DEFAULT_BASEPATH				"/usr/share/games/doom3bfg"
#endif
// RB end


