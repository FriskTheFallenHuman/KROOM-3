/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012-2014 Robert Beckebans
Copyright (C) 2013 Daniel Gibson

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#include "../RenderCommon.h"
#include "../RenderContext.h"
#include "../../sys/sdl/DeviceManagerLocal.h"
#include "../../sys/sdl/DeviceManagerAPI.h"

static SDL_GLContext glContext = NULL;

// RB: FIXME this shit. We need the OpenGL alpha channel for advanced rendering effects
static idCVar sdl_waylandcompat( "sdl_waylandcompat", "0", CVAR_SYSTEM | CVAR_NOCHEAT | CVAR_ARCHIVE, "wayland compatible framebuffer" );

/*
========================
GLVersionToDriverVersion
========================
*/
static graphicsVersionType_t GLVersionToDriverVersion( int major, int minor )
{
	static const graphicsVersionType_t gl4x[] =
	{
		GL_OPENGL_40, GL_OPENGL_41, GL_OPENGL_42, GL_OPENGL_43,
		GL_OPENGL_44, GL_OPENGL_45, GL_OPENGL_46
	};

	if( major >= 4 )
	{
		return gl4x[ idMath::ClampInt( 0, 6, minor ) ];
	}

	// 3.1 is the minimum, 3.3 the maximum of the 3.x series
	return ( minor >= 3 ) ? GL_OPENGL_33 : ( ( minor == 2 ) ? GL_OPENGL_32 : GL_OPENGL_31 );
}


/*
========================
DM_PrepareWindow
========================
*/
Uint32 DM_PrepareWindow( const vidParms_t& parms, int channelcolorbits, int tdepthbits, int tstencilbits )
{
	Uint32 driverFlag = 0;

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, channelcolorbits );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, channelcolorbits );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, channelcolorbits );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );

	if( sdl_waylandcompat.GetBool() )
	{
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
	}
	else
	{
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, channelcolorbits );
	}

	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples );

	const int glMajorVersion = r_glVersionMajor.GetInteger();
	const int glMinorVersion = r_glVersionMinor.GetInteger();

	glConfig.driverVersion = GLVersionToDriverVersion( glMajorVersion, glMinorVersion );

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, glMajorVersion );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, glMinorVersion );

	int useCoreProfile = r_glProfile.GetInteger();
	int profileCore = 0;

	if( useCoreProfile == 2 )
	{
		glConfig.driverType = GLDRV_OPENGL_CORE_PROFILE;
		profileCore = SDL_GL_CONTEXT_PROFILE_CORE;
	}
	else if( useCoreProfile == 1 )
	{
		glConfig.driverType = GLDRV_OPENGL_COMPATIBILITY_PROFILE;
		profileCore = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
	}
	else
	{
		glConfig.driverType = GLDRV_OPENGL;
		profileCore = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY; // ES support?
	}

// Profiles were introduced with OpenGL 3.2, for older versions the mask is meaningless
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, ( glConfig.driverVersion != GL_OPENGL_31 ) ? profileCore : 0 ); // ES support?

	if( r_debugContext.GetBool() )
	{
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
	}

	driverFlag = SDL_WINDOW_OPENGL;

	return driverFlag;
}

/*
========================
DM_CreateContext
========================
*/
bool DM_CreateContext( SDL_Window* window )
{
	glContext = SDL_GL_CreateContext( window );

	if( !glContext )
	{
		common->Warning( "Couldn't create an OpenGL %d.%d context (profile %d): %s",
						 r_glVersionMajor.GetInteger(), r_glVersionMinor.GetInteger(), r_glProfile.GetInteger(), SDL_GetError() );
		return false;
	}

	if( SDL_GL_SetSwapInterval( r_swapInterval.GetInteger() ) < 0 )
	{
		common->Warning( "SDL_GL_SetSwapInterval(%d) failed: %s", r_swapInterval.GetInteger(), SDL_GetError() );
	}

	return true;
}

/*
========================
DM_FinishInit
========================
*/
void DM_FinishInit( SDL_Window* window )
{
	int gladResult = gladLoadGL( ( GLADloadfunc )SDL_GL_GetProcAddress );
	if( !gladResult )
	{
		// gladLoadGL failed, something is seriously wrong
		common->FatalError( "^3GLimp_Init() - GLAD could not load OpenGL subsystem" );
	}
	else
	{
		common->Printf( "Using GLAD %d.%d\n", GLAD_VERSION_MAJOR( gladResult ), GLAD_VERSION_MINOR( gladResult ) );
	}

	int originalInterval = SDL_GL_GetSwapInterval();
	if( SDL_GL_SetSwapInterval( -1 ) == 0 )
	{
		glConfig.swapControlTearAvailable = true;
		SDL_GL_SetSwapInterval( originalInterval );
	}
	else
	{
		glConfig.swapControlTearAvailable = false;
	}

	rRenderContext.InitContext( window, glContext );
}

void DM_SetMultisamples( int multiSamples )
{
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, multiSamples ? 1 : 0 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, multiSamples );
}

const char* DM_APIName()
{
	return "OpenGL";
}

void DM_DestroyContext()
{
	if( glContext )
	{
		SDL_GL_DeleteContext( glContext );
		glContext = NULL;
	}
}

void DM_ShutdownSDL( bool shutdownSDL )
{
}

/*
========================
DM_SwapBuffers
========================
*/
void DM_SwapBuffers( SDL_Window* window )
{
	if( r_swapInterval.IsModified() )
	{
		r_swapInterval.ClearModified();

		int interval = r_swapInterval.GetInteger();
		if( interval == 1 && glConfig.swapControlTearAvailable )
		{
			interval = -1;
		}
		if( SDL_GL_SetSwapInterval( interval ) < 0 )
		{
			common->Warning( "Failed to set swap interval %d: %s", interval, SDL_GetError() );
		}
	}

	SDL_GL_SwapWindow( window );
}

/*
========================
DM_TestSwapBuffers
========================
*/
void DM_TestSwapBuffers( SDL_Window* window )
{
	common->Printf( "idDeviceManagerLocal::TestSwapBuffers\n" );
	static const int MAX_FRAMES = 5;
	uint64 timestamps[MAX_FRAMES];

	int oldInterval = SDL_GL_GetSwapInterval();

	// Disable scissor test for consistent timing
	GLboolean scissorEnabled = glIsEnabled( GL_SCISSOR_TEST );
	if( scissorEnabled )
	{
		glDisable( GL_SCISSOR_TEST );
	}

	// Test intervals: -1 (adaptive vsync if available), 0 (vsync off), 1 (vsync on), 2 (double vsync)
	int intervals[] = { -1, 0, 1, 2 };
	for( int idx = 0; idx < 4; idx++ )
	{
		int swapInterval = intervals[idx];

		// Try to set the interval; skip if unsupported
		if( SDL_GL_SetSwapInterval( swapInterval ) != 0 )
		{
			common->Printf( "Swap interval %d not supported: %s\n", swapInterval, SDL_GetError() );
			continue;
		}

		// Allow a few frames to settle
		for( int i = 0; i < MAX_FRAMES; i++ )
		{
			if( swapInterval == -1 && i == 0 )
			{
				SDL_Delay( 16 );    // ~60 Hz frame time
			}

			if( i & 1 )
			{
				glClearColor( 0, 1, 0, 1 );
			}
			else
			{
				glClearColor( 1, 0, 0, 1 );
			}

			glClear( GL_COLOR_BUFFER_BIT );
			SDL_GL_SwapWindow( window );
			glFinish();

			timestamps[i] = SDL_GetPerformanceCounter();
		}

		common->Printf( "\nswapinterval %i\n", swapInterval );
		uint64 freq = SDL_GetPerformanceFrequency();
		for( int i = 1; i < MAX_FRAMES; i++ )
		{
			uint64 dt = timestamps[i] - timestamps[i - 1];
			int microseconds = ( int )( ( dt * 1000000 ) / freq );
			common->Printf( "%i microseconds\n", microseconds );
		}
	}

	SDL_GL_SetSwapInterval( oldInterval );

	if( scissorEnabled )
	{
		glEnable( GL_SCISSOR_TEST );
	}
}
