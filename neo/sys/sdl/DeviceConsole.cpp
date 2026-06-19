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

#include "precompiled.h"
#pragma hdrstop

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL2/SDL.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"

#include "DeviceConsole.h"

DeviceConsoleVar_t	deviceConsole = {};

static const int PEEP_BATCH_SIZE = 64;

/*
=============
DeviceConsoleVar_t::Print
=============
*/
void DeviceConsoleVar_t::Print( const char* text )
{
	if( !g_active || !text || !text[ 0 ] )
	{
		return;
	}

	const int len = ( int )strlen( text );
	if( len == 0 )
	{
		return;
	}

	SDL_LockMutex( g_logMutex );

	// If the buffer would overflow, discard the oldest half.
	if( g_logLen + len >= MAX_LOG_BYTES )
	{
		const int half = MAX_LOG_BYTES / 2;
		memmove( g_logBuf, g_logBuf + half, g_logLen - half );
		g_logLen -= half;
	}

	memcpy( g_logBuf + g_logLen, text, len );
	g_logLen += len;
	g_logBuf[ g_logLen ] = '\0';
	g_scrollToBot = true;

	SDL_UnlockMutex( g_logMutex );
}

/*
=============
DeviceConsoleVar_t::ClearLog
=============
*/
void DeviceConsoleVar_t::ClearLog()
{
	SDL_LockMutex( g_logMutex );
	g_logLen = 0;
	g_logBuf[0] = '\0';
	SDL_UnlockMutex( g_logMutex );
}

/*
=============
DeviceConsoleVar_t::IsConsoleEvent
=============
*/
bool DeviceConsoleVar_t::IsConsoleEvent( const SDL_Event& ev, Uint32 winID )
{
	switch( ev.type )
	{
		case SDL_WINDOWEVENT:
			return ev.window.windowID == winID;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			return ev.key.windowID == winID;
		case SDL_TEXTINPUT:
			return ev.text.windowID == winID;
		case SDL_MOUSEMOTION:
			return ev.motion.windowID == winID;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			return ev.button.windowID == winID;
		case SDL_MOUSEWHEEL:
			return ev.wheel.windowID == winID;
		default:
			return false;
	}
}

/*
=============
DeviceConsoleVar_t::PumpOwnEvents
=============
*/
void DeviceConsoleVar_t::PumpOwnEvents( Uint32 winID )
{
	SDL_Event batch[ PEEP_BATCH_SIZE ];
	int count = SDL_PeepEvents( batch, PEEP_BATCH_SIZE, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT );
	if( count <= 0 )
	{
		return;
	}

	for( int i = 0; i < count; i++ )
	{
		if( IsConsoleEvent( batch[ i ], winID ) )
		{
			ImGui_ImplSDL2_ProcessEvent( &batch[ i ] );

			if( batch[ i ].type == SDL_WINDOWEVENT &&
					batch[ i ].window.event == SDL_WINDOWEVENT_CLOSE )
			{
				if( g_quitOnClose )
				{
					SDL_AtomicSet( &g_quitRequested, 1 );
				}
				else
				{
					SDL_HideWindow( g_window );
					g_visible = false;
				}
			}
		}
		else
		{
			SDL_PushEvent( &batch[ i ] );
		}
	}
}

/*
=============
HistoryCallback

ImGui InputText callback for command history navigation.
=============
*/
static int HistoryCallback( ImGuiInputTextCallbackData* data )
{
	if( data->EventFlag != ImGuiInputTextFlags_CallbackHistory )
	{
		return 0;
	}

	const int prev = deviceConsole.g_histPos;

	if( data->EventKey == ImGuiKey_UpArrow )
	{
		if( deviceConsole.g_histPos == -1 )
		{
			deviceConsole.g_histPos = deviceConsole.g_histCount - 1;
		}
		else if( deviceConsole.g_histPos > 0 )
		{
			deviceConsole.g_histPos--;
		}
	}
	else if( data->EventKey == ImGuiKey_DownArrow )
	{
		if( deviceConsole.g_histPos != -1 && ++deviceConsole.g_histPos >= deviceConsole.g_histCount )
		{
			deviceConsole.g_histPos = -1;
		}
	}

	if( prev != deviceConsole.g_histPos )
	{
		const char* entry = ( deviceConsole.g_histPos >= 0 ) ? deviceConsole.g_history[ deviceConsole.g_histPos ] : "";
		data->DeleteChars( 0, data->BufTextLen );
		data->InsertChars( 0, entry );
	}

	return 0;
}

/*
=============
ConsoleThreadFunc
=============
*/
static int ConsoleThreadFunc( void* )
{
#ifndef __APPLE__
	deviceConsole.g_window = SDL_CreateWindow( GAME_NAME,
							 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							 CON_WIN_W, CON_WIN_H,
							 SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN );

	if( deviceConsole.g_window )
	{
		deviceConsole.g_renderer = SDL_CreateRenderer( deviceConsole.g_window, -1, SDL_RENDERER_ACCELERATED );
		if( !deviceConsole.g_renderer )
		{
			deviceConsole.g_renderer = SDL_CreateRenderer( deviceConsole.g_window, -1, SDL_RENDERER_SOFTWARE );
		}
		if( !deviceConsole.g_renderer )
		{
			SDL_DestroyWindow( deviceConsole.g_window );
			deviceConsole.g_window = nullptr;
		}
	}

	if( !deviceConsole.g_window || !deviceConsole.g_renderer )
	{
		fprintf( stderr, "ConsoleThreadFunc: window/renderer creation failed: %s\n", SDL_GetError() );
		SDL_SemPost( deviceConsole.g_initSem );
		return 1;
	}
#endif

	// This thread owns the console ImGui context exclusively.
	deviceConsole.g_Consolectx = ImGui::CreateContext();
	ImGui::SetCurrentContext( deviceConsole.g_Consolectx );

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;	// no layout persistence

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.FrameRounding = 2.0f;
	style.ScrollbarRounding	= 2.0f;
	style.Colors[ ImGuiCol_WindowBg ] = ImVec4( 0.08f, 0.08f, 0.08f, 1.0f );
	style.Colors[ ImGuiCol_FrameBg ] = ImVec4( 0.14f, 0.14f, 0.14f, 1.0f );
	style.Colors[ ImGuiCol_ChildBg ] = ImVec4( 0.06f, 0.06f, 0.06f, 1.0f );

	ImGui_ImplSDL2_InitForSDLRenderer( deviceConsole.g_window, deviceConsole.g_renderer );
	ImGui_ImplSDLRenderer2_Init( deviceConsole.g_renderer );

	const Uint32 winID = SDL_GetWindowID( deviceConsole.g_window );
	char inputBuf[ MAX_CMD_LEN ] = {};

	SDL_SemPost( deviceConsole.g_initSem );

	while( SDL_AtomicGet( &deviceConsole.g_running ) )
	{
		const Uint32 frameStart = SDL_GetTicks();

		// Event handling
		SDL_PumpEvents();
		deviceConsole.PumpOwnEvents( winID );

		// Frame
		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		const ImVec2 displaySize = io.DisplaySize;
		ImGui::SetNextWindowPos( ImVec2( 0.0f, 0.0f ) );
		ImGui::SetNextWindowSize( displaySize );
		ImGui::Begin( "##syscon", nullptr,
					  ImGuiWindowFlags_NoTitleBar  |
					  ImGuiWindowFlags_NoResize    |
					  ImGuiWindowFlags_NoMove      |
					  ImGuiWindowFlags_NoCollapse  |
					  ImGuiWindowFlags_NoSavedSettings );

		// Scrollable log area — reserve space for the input row
		const float inputRowH = ImGui::GetFrameHeightWithSpacing() * 2.0f + 6.0f;
		ImGui::BeginChild( "##log",
						   ImVec2( 0.0f, -inputRowH ),
						   false,
						   ImGuiWindowFlags_HorizontalScrollbar );

		SDL_LockMutex( deviceConsole.g_logMutex );

		const char* lineStart = deviceConsole.g_logBuf;
		const char* bufEnd    = deviceConsole.g_logBuf + deviceConsole.g_logLen;

		while( lineStart <= bufEnd )
		{
			const char* lineEnd   = ( const char* )memchr( lineStart, '\n', bufEnd - lineStart );
			const char* lineLimit = lineEnd ? lineEnd : bufEnd;

			// Render one line, switching color at each ^N marker.
			ImVec4 curColor = ImGui::GetStyle().Colors[ ImGuiCol_Text ];
			const char* runStart = lineStart;
			const char* p = lineStart;
			bool first = true;

			while( p <= lineLimit )
			{
				bool atMarker = ( p + 1 < lineLimit && *p == '^' && p[1] >= '0' && p[1] <= '9' );
				bool atEnd    = ( p == lineLimit );

				if( atMarker || atEnd )
				{
					if( p > runStart )
					{
						if( !first )
						{
							ImGui::SameLine( 0.0f, 0.0f );
						}
						ImGui::PushStyleColor( ImGuiCol_Text, curColor );
						ImGui::TextUnformatted( runStart, p );
						ImGui::PopStyleColor();
						first = false;
					}

					if( atMarker )
					{
						int idx = p[1] - '0';
						// ^0 means "reset", not "draw black" — g_color_table[0]
						// is literally black, which isn't the intent here.
						if( idx == 0 )
						{
							curColor = ImGui::GetStyle().Colors[ ImGuiCol_Text ];
						}
						else
						{
							idVec4& c = idStr::ColorForIndex( idx );
							curColor = ImVec4( c.x, c.y, c.z, c.w );
						}
						p += 2;
						runStart = p;
						continue;
					}

					break; // atEnd
				}

				p++;
			}

			if( first )
			{
				ImGui::NewLine(); // empty line still needs its height
			}

			if( !lineEnd )
			{
				break;
			}
			lineStart = lineEnd + 1;
		}

		const bool doScroll = deviceConsole.g_scrollToBot;
		deviceConsole.g_scrollToBot = false;

		if( doScroll )
		{
			ImGui::SetScrollHereY( 1.0f ); // must run before EndChild
		}

		SDL_UnlockMutex( deviceConsole.g_logMutex );

		ImGui::EndChild();
		ImGui::Separator();

		// Input field
		ImGui::PushItemWidth( -1.0f );
		bool reclaim = false;

		if( ImGui::InputText( "##cmd", inputBuf, MAX_CMD_LEN,
							  ImGuiInputTextFlags_EnterReturnsTrue |
							  ImGuiInputTextFlags_CallbackHistory,
							  HistoryCallback ) )
		{
			if( inputBuf[ 0 ] != '\0' )
			{
				// Add to history
				if( deviceConsole.g_histCount < MAX_HISTORY )
				{
					SDL_strlcpy( deviceConsole.g_history[ deviceConsole.g_histCount++ ], inputBuf, MAX_CMD_LEN );
				}
				else
				{
					memmove( deviceConsole.g_history[ 0 ], deviceConsole.g_history[ 1 ],
							 sizeof( deviceConsole.g_history ) - sizeof( deviceConsole.g_history[ 0 ] ) );
					SDL_strlcpy( deviceConsole.g_history[ MAX_HISTORY - 1 ], inputBuf, MAX_CMD_LEN );
				}
				deviceConsole.g_histPos = -1;

				// Echo the typed command into the log.
				char echoLine[ MAX_CMD_LEN + 8 ];
				SDL_snprintf( echoLine, sizeof( echoLine ), "]%s\n", inputBuf );
				deviceConsole.Print( echoLine );

				// Queue for the engine to consume via Sys_ConsoleInput()
				SDL_LockMutex( deviceConsole.g_inputMutex );
				if( !deviceConsole.g_hasInput )
				{
					SDL_strlcpy( deviceConsole.g_inputPending, inputBuf, MAX_CMD_LEN );
					deviceConsole.g_hasInput = true;
				}
				SDL_UnlockMutex( deviceConsole.g_inputMutex );

				inputBuf[ 0 ] = '\0';
			}
			reclaim = true;
		}

		ImGui::PopItemWidth();
		ImGui::SetItemDefaultFocus();
		if( reclaim )
		{
			ImGui::SetKeyboardFocusHere( -1 );
		}

		// Clear & Copy Buttons
		if( ImGui::Button( "Copy All" ) )
		{
			SDL_LockMutex( deviceConsole.g_logMutex );
			ImGui::SetClipboardText( deviceConsole.g_logBuf );
			SDL_UnlockMutex( deviceConsole.g_logMutex );
		}
		ImGui::SameLine();
		if( ImGui::Button( "Clear" ) )
		{
			deviceConsole.ClearLog();
		}

		ImGui::End();

		// Render
		ImGui::Render();
		SDL_SetRenderDrawColor( deviceConsole.g_renderer, 18, 18, 18, 255 );
		SDL_RenderClear( deviceConsole.g_renderer );
		ImGui_ImplSDLRenderer2_RenderDrawData( ImGui::GetDrawData(), deviceConsole.g_renderer );
		SDL_RenderPresent( deviceConsole.g_renderer );

		// Cap us, since this its just an output windows
		const Uint32 elapsed = SDL_GetTicks() - frameStart;
		if( elapsed < FRAME_TARGET_MS )
		{
			SDL_Delay( FRAME_TARGET_MS - elapsed );
		}
	}

	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext( deviceConsole.g_Consolectx );
	deviceConsole.g_Consolectx = nullptr;

#ifndef __APPLE__
	SDL_DestroyRenderer( deviceConsole.g_renderer );
	SDL_DestroyWindow( deviceConsole.g_window );
	deviceConsole.g_renderer = nullptr;
	deviceConsole.g_window   = nullptr;
#endif

	return 0;
}

/*
=============
DeviceConsoleVar_t::CreateConsole
=============
*/
void DeviceConsoleVar_t::CreateConsole()
{
	if( g_active )
	{
		return;
	}

	if( SDL_InitSubSystem( SDL_INIT_VIDEO ) != 0 )
	{
		fprintf( stderr, "DeviceConsoleVar_t::CreateConsole: SDL_InitSubSystem(VIDEO) failed: %s\n",
				 SDL_GetError() );
		return;
	}

	g_logMutex   = SDL_CreateMutex();
	g_inputMutex = SDL_CreateMutex();
	g_initSem    = SDL_CreateSemaphore( 0 );

	if( !g_logMutex || !g_inputMutex || !g_initSem )
	{
		fprintf( stderr, "DeviceConsoleVar_t::CreateConsole: SDL_CreateMutex/Semaphore failed: %s\n",
				 SDL_GetError() );
		return;
	}

	// Reset state
	memset( g_logBuf, 0, sizeof( g_logBuf ) );
	memset( g_inputPending, 0, sizeof( g_inputPending ) );
	g_logLen = 0;
	g_hasInput = false;
	g_histCount = 0;
	g_histPos = -1;
	g_visible = false;
	g_quitOnClose = false;

	SDL_AtomicSet( &g_running,       1 );
	SDL_AtomicSet( &g_quitRequested, 0 );

#ifdef __APPLE__
	g_window = SDL_CreateWindow( GAME_NAME,
								 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
								 CON_WIN_W, CON_WIN_H,
								 SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN );
	if( !g_window )
	{
		fprintf( stderr, "DeviceConsoleVar_t::CreateConsole: SDL_CreateWindow failed: %s\n",
				 SDL_GetError() );
		return;
	}

	g_renderer = SDL_CreateRenderer( g_window, -1, SDL_RENDERER_ACCELERATED );
	if( !g_renderer )
	{
		g_renderer = SDL_CreateRenderer( g_window, -1, SDL_RENDERER_SOFTWARE );
	}
	if( !g_renderer )
	{
		fprintf( stderr, "DeviceConsoleVar_t::CreateConsole: SDL_CreateRenderer failed: %s\n",
				 SDL_GetError() );
		SDL_DestroyWindow( g_window );
		g_window = nullptr;
		return;
	}
#endif

	g_thread = SDL_CreateThread( ConsoleThreadFunc, "SysConsole", nullptr );
	if( !g_thread )
	{
		fprintf( stderr, "DeviceConsoleVar_t::CreateConsole: SDL_CreateThread failed: %s\n",
				 SDL_GetError() );
#ifdef __APPLE__
		SDL_DestroyRenderer( g_renderer );
		SDL_DestroyWindow( g_window );
		g_renderer = nullptr;
		g_window   = nullptr;
#endif
		return;
	}

	SDL_SemWait( g_initSem );

	if( !g_window )
	{
		return;
	}

	g_active = true;
}

/*
=============
DeviceConsoleVar_t::DestroyConsole
=============
*/
void DeviceConsoleVar_t::DestroyConsole()
{
	if( !g_active )
	{
		return;
	}

	SDL_AtomicSet( &g_running, 0 );

	if( g_thread )
	{
		SDL_WaitThread( g_thread, nullptr );
		g_thread = nullptr;
	}

#ifdef __APPLE__
	SDL_DestroyRenderer( g_renderer );
	SDL_DestroyWindow( g_window );
	g_renderer = nullptr;
	g_window   = nullptr;
#endif

	SDL_DestroyMutex( g_logMutex );
	SDL_DestroyMutex( g_inputMutex );
	SDL_DestroySemaphore( g_initSem );
	g_logMutex = nullptr;
	g_inputMutex = nullptr;
	g_initSem = nullptr;

	g_active = false;
	g_visible = false;
}

/*
=============
DeviceConsoleVar_t::ConsoleInput
=============
*/
char* DeviceConsoleVar_t::ConsoleInput()
{
	if( !g_active )
	{
		return nullptr;
	}

	SDL_LockMutex( g_inputMutex );

	char* result = nullptr;
	if( g_hasInput )
	{
		static char returnBuf[ MAX_CMD_LEN ];
		SDL_strlcpy( returnBuf, g_inputPending, MAX_CMD_LEN );
		g_hasInput = false;
		result     = returnBuf;
	}

	SDL_UnlockMutex( g_inputMutex );

	return result;
}

/*
=============
DeviceConsoleVar_t::ShowConsole
=============
*/
void DeviceConsoleVar_t::ShowConsole( int visLevel, bool quitOnClose )
{
	if( !g_active || !g_window )
	{
		return;
	}

	g_quitOnClose = quitOnClose;

	switch( visLevel )
	{
		case 0: // hide
			if( g_visible )
			{
				SDL_HideWindow( g_window );
				g_visible = false;
			}
			break;
		case 1: // show normal
			if( !g_visible )
			{
				SDL_ShowWindow( g_window );
				g_visible = true;
			}
			break;
		case 2: // minimise (optional)
			SDL_MinimizeWindow( g_window );
			break;
		default:
			break;
	}
}

/*
=============
DeviceConsoleVar_t::IsConsoleVisible
=============
*/
bool DeviceConsoleVar_t::IsConsoleVisible()
{
	return g_visible;
}

/*
=============
DeviceConsoleVar_t::IsQuitRequested
=============
*/
bool DeviceConsoleVar_t::IsQuitRequested()
{
	return SDL_AtomicGet( &g_quitRequested ) != 0;
}

/*
=============
Sys_CreateConsole
=============
*/
void Sys_CreateConsole()
{
	deviceConsole.CreateConsole();
}

/*
=============
Sys_ShowConsole
=============
*/
void Sys_ShowConsole( int visLevel, bool quitOnClose )
{
	deviceConsole.ShowConsole( visLevel, quitOnClose );
}

/*
=============
Sys_DestroyConsole
=============
*/
void Sys_DestroyConsole()
{
	deviceConsole.DestroyConsole();
}


/*
=============
Sys_ConsoleInput
=============
*/
char* Sys_ConsoleInput()
{
	return deviceConsole.ConsoleInput();
}

/*
=============
Sys_IsConsoleVisible
=============
*/
bool Sys_IsConsoleVisible()
{
	return deviceConsole.IsConsoleVisible();
}