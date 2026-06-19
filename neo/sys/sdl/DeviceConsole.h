/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __DEVICECONSOLE_H__
#define __DEVICECONSOLE_H__

struct ImGuiContext;

const int CON_WIN_W			= 720;
const int CON_WIN_H			= 460;
const int MAX_LOG_BYTES		= 512 * 1024;	// 512 KB rolling buffer
const int MAX_CMD_LEN		= 512;
const int MAX_HISTORY		= 32;
const Uint32 FRAME_TARGET_MS	= 33;			// ~30 fps

struct DeviceConsoleVar_t
{
	DeviceConsoleVar_t()
	{
		g_logLen = 0;
		g_scrollToBot = false;
		g_logMutex = nullptr;
		g_hasInput = false;
		g_inputMutex = nullptr;
		g_histCount = 0;
		g_histPos = -1;	// -1 = editing current line
		g_window = nullptr;
		g_renderer = nullptr;
		g_Consolectx = nullptr;
		g_thread = nullptr;
		g_initSem = nullptr;
		g_active = false;
		g_visible = false;
		g_quitOnClose = false;
	}

	void Print( const char* text );
	void ClearLog();
	bool IsConsoleEvent( const SDL_Event& ev, Uint32 winID );
	void PumpOwnEvents( Uint32 winID );
	bool IsQuitRequested();

	// for their Sys_ counter part
	void CreateConsole();
	void DestroyConsole();
	void ShowConsole( int visLevel, bool quitOnClose );
	bool IsConsoleVisible();
	char* ConsoleInput();

public:
	// Log buffer
	char		g_logBuf[ MAX_LOG_BYTES + 1 ];
	int			g_logLen;
	bool		g_scrollToBot;
	SDL_mutex*	g_logMutex;

	// Input
	char		g_inputPending[ MAX_CMD_LEN ];
	bool		g_hasInput;
	SDL_mutex*	g_inputMutex;

	// Console history
	char		g_history[ MAX_HISTORY ][ MAX_CMD_LEN ];
	int			g_histCount;
	int			g_histPos;

	// SDL stuffs
	SDL_Window*		g_window;
	SDL_Renderer*	g_renderer;

	// ImGui
	ImGuiContext*	g_Consolectx;

	// Thread control
	SDL_Thread*		g_thread;
	SDL_atomic_t	g_running;
	SDL_sem*		g_initSem;

	SDL_atomic_t	g_quitRequested;

	bool	g_active;
	bool	g_visible;
	bool	g_quitOnClose;
};

extern DeviceConsoleVar_t	deviceConsole;

#endif /* !__DEVICECONSOLE_H__ */