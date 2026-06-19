/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#include "precompiled.h"
#pragma hdrstop

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"

#if defined(__linux__)
	#include <sys/utsname.h>
	#include <fstream>
	#include <unistd.h>
#elif defined(__APPLE__)
	#include <sys/types.h>
	#include <sys/sysctl.h>
	#include <unistd.h>
	#include <mach-o/dyld.h>
#endif

#include "../sys_local.h"
#include "../sdl/DeviceSDL.h"

/*
=============
Sys_ShowCrashDialog
=============
*/
void Sys_ShowCrashDialog( const char* summaryText )
{
	if( SDL_WasInit( SDL_INIT_VIDEO ) == 0 &&
			SDL_InitSubSystem( SDL_INIT_VIDEO ) != 0 )
	{
		return; // nothing safe left to do
	}

	SDL_Window* win = SDL_CreateWindow( "Unhandled Exception",
										SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 560, 360, SDL_WINDOW_SHOWN );
	if( !win )
	{
		return;
	}

	SDL_Renderer* ren = SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED );
	if( !ren )
	{
		ren = SDL_CreateRenderer( win, -1, SDL_RENDERER_SOFTWARE );
	}
	if( !ren )
	{
		SDL_DestroyWindow( win );
		return;
	}

	ImGuiContext* ctx = ImGui::CreateContext();
	ImGui::SetCurrentContext( ctx );
	ImGui::GetIO().IniFilename = nullptr;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForSDLRenderer( win, ren );
	ImGui_ImplSDLRenderer2_Init( ren );

	bool open = true;
	while( open )
	{
		SDL_Event ev;
		while( SDL_PollEvent( &ev ) )
		{
			ImGui_ImplSDL2_ProcessEvent( &ev );
			if( ev.type == SDL_QUIT ||
					( ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_CLOSE ) )
			{
				open = false;
			}
		}

		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
		ImGui::SetNextWindowSize( ImGui::GetIO().DisplaySize );
		ImGui::Begin( "##crash", nullptr,
					  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
					  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

		ImGui::TextWrapped( "%s", summaryText );
		ImGui::Spacing();
		if( ImGui::Button( "Close", ImVec2( 120, 0 ) ) )
		{
			open = false;
		}

		ImGui::End();
		ImGui::Render();
		SDL_SetRenderDrawColor( ren, 24, 24, 24, 255 );
		SDL_RenderClear( ren );
		ImGui_ImplSDLRenderer2_RenderDrawData( ImGui::GetDrawData(), ren );
		SDL_RenderPresent( ren );
		SDL_Delay( 16 );
	}

	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext( ctx );
	SDL_DestroyRenderer( ren );
	SDL_DestroyWindow( win );
}