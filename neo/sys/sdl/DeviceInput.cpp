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

#include <SDL2/SDL.h>

#include "DeviceSDL.h"

/*
================
idJoystickSDL::idJoystickSDL
================
*/
idJoystickSDL::idJoystickSDL()
	: m_joy( nullptr )
	, m_hasHat( 0 )
{
	m_pollEvents.SetGranularity( 64 );
}

/*
================
idJoystickSDL::~idJoystickSDL
================
*/
idJoystickSDL::~idJoystickSDL()
{
	Shutdown();
}

/*
================
idJoystickSDL::Init
================
*/
bool idJoystickSDL::Init()
{
	// Already initialised?
	if( m_joy )
	{
		return true;
	}

	// Initialise SDL joystick subsystem if needed
	if( SDL_WasInit( SDL_INIT_JOYSTICK ) == 0 )
	{
		if( SDL_Init( SDL_INIT_JOYSTICK ) )
		{
			common->Printf( "idJoystickSDL::Init: SDL_INIT_JOYSTICK error: %s\n", SDL_GetError() );
			return false;
		}
	}

	int numJoysticks = SDL_NumJoysticks();
	common->Printf( "idJoystickSDL::Init: Found %i joysticks\n", numJoysticks );

	if( numJoysticks > 0 )
	{
		m_joy = SDL_JoystickOpen( 0 );
		if( m_joy )
		{
			int numHats = SDL_JoystickNumHats( m_joy );
			common->Printf( "Opened Joystick number 0\n" );
			common->Printf( "Name: %s\n", SDL_JoystickName( m_joy ) );
			common->Printf( "Number of Axes: %d\n", SDL_JoystickNumAxes( m_joy ) );
			common->Printf( "Number of Buttons: %d\n", SDL_JoystickNumButtons( m_joy ) );
			common->Printf( "Number of Hats: %d\n", numHats );
			common->Printf( "Number of Balls: %d\n", SDL_JoystickNumBalls( m_joy ) );

			m_hasHat = ( numHats > 0 ) ? 1 : 0;
		}
		else
		{
			common->Printf( "Couldn't open Joystick 0\n" );
			m_joy = nullptr;
		}
	}
	else
	{
		common->Printf( "No joysticks found.\n" );
		m_joy = nullptr;
	}

	// Initialise game controller subsystem for extended mapping (optional)
	if( SDL_WasInit( SDL_INIT_GAMECONTROLLER ) == 0 )
	{
		if( SDL_Init( SDL_INIT_GAMECONTROLLER ) )
		{
			common->Printf( "idJoystickSDL::Init: SDL_INIT_GAMECONTROLLER error: %s\n", SDL_GetError() );
		}
		else
		{
			// Enumerate and print game controllers (debugging)
			for( int i = 0; i < SDL_NumJoysticks(); ++i )
			{
				if( SDL_IsGameController( i ) )
				{
					SDL_GameController* controller = SDL_GameControllerOpen( i );
					if( controller )
					{
						common->Printf( "GameController %i name: %s\n", i, SDL_GameControllerName( controller ) );
						common->Printf( "GameController %i is mapped as \"%s\".\n", i, SDL_GameControllerMapping( controller ) );
						SDL_GameControllerClose( controller );
					}
					else
					{
						common->Printf( "Could not open gamecontroller %i: %s\n", i, SDL_GetError() );
					}
				}
			}
		}
	}

	return ( m_joy != nullptr );
}

/*
================
idJoystickSDL::Shutdown
================
*/
void idJoystickSDL::Shutdown()
{
	if( m_joy )
	{
		common->Printf( "idJoystickSDL::Shutdown: closing SDL joystick.\n" );
		SDL_JoystickClose( m_joy );
		m_joy = nullptr;
	}
	m_pollEvents.Clear();
	m_hasHat = 0;
}

/*
================
idJoystickSDL::SetRumble
================
*/
void idJoystickSDL::SetRumble( int deviceNum, int rumbleLow, int rumbleHigh )
{
	// TODO
}

/*
================
idJoystickSDL::PollInputEvents
================
*/
int idJoystickSDL::PollInputEvents( int inputDeviceNum )
{
	// Returns the number of raw events waiting
	return m_pollEvents.Num();
}

/*
================
idJoystickSDL::ReturnInputEvent
================
*/
int idJoystickSDL::ReturnInputEvent( const int n, int& action, int& value )
{
	if( n < 0 || n >= m_pollEvents.Num() )
	{
		return 0;
	}

	action = m_pollEvents[n].action;
	value  = m_pollEvents[n].value;
	return 1;
}

/*
================
idJoystickSDL::EndInputEvents
================
*/
void idJoystickSDL::EndInputEvents()
{
	// Clear the queue after all events have been read
	m_pollEvents.Clear();
}

/*
================
idJoystickSDL::AddPollEvent
================
*/
void idJoystickSDL::AddPollEvent( int action, int value )
{
	m_pollEvents.Append( JoystickPollEvent( action, value ) );
}

//=====================================================================================
//	Joystick Input Handling
//=====================================================================================

void Sys_SetRumble( int device, int low, int hi )
{
	return sdl.g_Joystick.SetRumble( device, low, hi );
}

int Sys_PollJoystickInputEvents( int deviceNum )
{
	return sdl.g_Joystick.PollInputEvents( deviceNum );
}

int Sys_ReturnJoystickInputEvent( const int n, int& action, int& value )
{
	return sdl.g_Joystick.ReturnInputEvent( n, action, value );
}

void Sys_EndJoystickInputEvents()
{
	sdl.g_Joystick.EndInputEvents();
}