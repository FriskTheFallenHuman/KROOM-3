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

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
// SRS - optinally needed for VK_MVK_MOLTENVK_EXTENSION_NAME visibility
#if defined(__APPLE__) && defined(USE_MoltenVK)
	#include <MoltenVK/vk_mvk_moltenvk.h>
#endif
#include <vector>

#include "renderer/RenderCommon.h"
#include "sdl_local.h"

idCVar in_nograb( "in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "prevents input grabbing" );

static bool grabbed = false;

//vulkanContext_t vkcontext; // Eric: I added this to pass SDL_Window* window to the SDL_Vulkan_* methods that are used else were.

static SDL_Window* window = nullptr;

// Eric: Integrate this into RBDoom3BFG's source code ecosystem.
// Helper function for using SDL2 and Vulkan on Linux.
std::vector<const char*> get_required_extensions()
{
	uint32_t                 sdlCount = 0;
	std::vector<const char*> sdlInstanceExtensions = {};

	SDL_Vulkan_GetInstanceExtensions( nullptr, &sdlCount, nullptr );
	sdlInstanceExtensions.resize( sdlCount );
	SDL_Vulkan_GetInstanceExtensions( nullptr, &sdlCount, sdlInstanceExtensions.data() );

	// SRS - Report enabled instance extensions in CreateVulkanInstance() vs. doing it here
	/*
	if( enableValidationLayers )
	{
		idLib::Printf( "\nNumber of availiable instance extensions\t%i\n", sdlCount );
		idLib::Printf( "Available Extension List: \n" );
		for( auto ext : sdlInstanceExtensions )
		{
			idLib::Printf( "\t%s\n", ext );
		}
	}
	*/

	// SRS - needed for MoltenVK portability implementation and optionally for MoltenVK configuration on OSX
#if defined(__APPLE__)
	sdlInstanceExtensions.push_back( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );
#if defined(USE_MoltenVK)
	sdlInstanceExtensions.push_back( VK_MVK_MOLTENVK_EXTENSION_NAME );
#endif
#endif

	// SRS - Add debug instance extensions in CreateVulkanInstance() vs. hardcoding them here
	/*
	if( enableValidationLayers )
	{
		sdlInstanceExtensions.push_back( "VK_EXT_debug_report" );
		sdlInstanceExtensions.push_back( "VK_EXT_debug_utils" );

		idLib::Printf( "Number of active instance extensions\t%zu\n", sdlInstanceExtensions.size() );
		idLib::Printf( "Active Extension List: \n" );
		for( auto const& ext : sdlInstanceExtensions )
		{
			idLib::Printf( "\t%s\n", ext );
		}
	}
	*/

	return sdlInstanceExtensions;
}

/*
===================
VKimp_PreInit

 R_GetModeListForDisplay is called before VKimp_Init(), but SDL needs SDL_Init() first.
 So do that in VKimp_PreInit()
 Calling that function more than once doesn't make a difference
===================
*/
void VKimp_PreInit() // DG: added this function for SDL compatibility
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		if( SDL_Init( SDL_INIT_VIDEO ) )
		{
			common->Error( "Error while initializing SDL: %s", SDL_GetError() );
		}
	}
}


/* Eric: Is the majority of this function not needed since switching from GL to Vulkan?
===================
VKimp_Init
===================
*/
bool VKimp_Init( glimpParms_t parms )
{

	common->Printf( "Initializing Vulkan subsystem\n" );

	VKimp_PreInit(); // DG: make sure SDL is initialized

	// DG: make window resizable
	Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
	// DG end

	if( parms.fullScreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;

	for( int i = 0; i < 16; i++ )
	{
		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if( ( i % 4 ) == 0 && i )
		{
			// one pass, reduce
			switch( i / 4 )
			{
				case 2 :
					if( colorbits == 24 )
					{
						colorbits = 16;
					}
					break;
				case 1 :
					if( depthbits == 24 )
					{
						depthbits = 16;
					}
					else if( depthbits == 16 )
					{
						depthbits = 8;
					}
				case 3 :
					if( stencilbits == 24 )
					{
						stencilbits = 16;
					}
					else if( stencilbits == 16 )
					{
						stencilbits = 8;
					}
			}
		}

		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;

		if( ( i % 4 ) == 3 )
		{
			// reduce colorbits
			if( tcolorbits == 24 )
			{
				tcolorbits = 16;
			}
		}

		if( ( i % 4 ) == 2 )
		{
			// reduce depthbits
			if( tdepthbits == 24 )
			{
				tdepthbits = 16;
			}
			else if( tdepthbits == 16 )
			{
				tdepthbits = 8;
			}
		}

		if( ( i % 4 ) == 1 )
		{
			// reduce stencilbits
			if( tstencilbits == 24 )
			{
				tstencilbits = 16;
			}
			else if( tstencilbits == 16 )
			{
				tstencilbits = 8;
			}
			else
			{
				tstencilbits = 0;
			}
		}

		int channelcolorbits = 4;
		if( tcolorbits == 24 )
		{
			channelcolorbits = 8;
		}

		// DG: set display num for fullscreen
		int windowPos = SDL_WINDOWPOS_UNDEFINED;
		if( parms.fullScreen > 0 )
		{
			if( parms.fullScreen > SDL_GetNumVideoDisplays() )
			{
				common->Warning( "Couldn't set display to num %i because we only have %i displays",
								 parms.fullScreen, SDL_GetNumVideoDisplays() );
			}
			else
			{
				// -1 because SDL starts counting displays at 0, while parms.fullScreen starts at 1
				windowPos = SDL_WINDOWPOS_UNDEFINED_DISPLAY( ( parms.fullScreen - 1 ) );
			}
		}
		// TODO: if parms.fullScreen == -1 there should be a borderless window spanning multiple displays
		/*
		 * NOTE that this implicitly handles parms.fullScreen == -2 (from r_fullscreen -2) meaning
		 * "do fullscreen, but I don't care on what monitor", at least on my box it's the monitor with
		 * the mouse cursor.
		 */

		window = SDL_CreateWindow( GAME_NAME,
								   windowPos,
								   windowPos,
								   parms.width, parms.height, flags );
		// DG end

		if( !window )
		{
			common->DPrintf( "Couldn't set Vulkan mode %d/%d/%d: %s",
							 channelcolorbits, tdepthbits, tstencilbits, SDL_GetError() );
			continue;
		}
		vkcontext.sdlWindow = window;
		// RB begin
		SDL_GetWindowSize( window, &glConfig.nativeScreenWidth, &glConfig.nativeScreenHeight );
		// RB end

		glConfig.isFullscreen = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) == SDL_WINDOW_FULLSCREEN;

		common->Printf( "Using %d color bits, %d depth, %d stencil display\n",
						channelcolorbits, tdepthbits, tstencilbits );

		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;

		// RB begin
		glConfig.displayFrequency = 60;
		glConfig.multisamples = parms.multiSamples;
		// RB end

		break;
	}

	if( !window )
	{
		common->Printf( "No usable VK mode found: %s", SDL_GetError() );
		return false;
	}

#if defined(__APPLE__)
	// SRS - On OSX enable SDL2 relative mouse mode warping to capture mouse properly if outside of window
	SDL_SetHintWithPriority( SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE );
#endif

	// DG: disable cursor, we have two cursors in menu (because mouse isn't grabbed in menu)
	SDL_ShowCursor( SDL_DISABLE );
	// DG end

	return true;
}

/*
===================
 Helper functions for VKimp_SetScreenParms()
===================
*/
static int ScreenParmsHandleDisplayIndex( glimpParms_t parms )
{
	int displayIdx;
	if( parms.fullScreen > 0 )
	{
		displayIdx = parms.fullScreen - 1; // first display for SDL is 0, in parms it's 1
	}
	else // -2 == use current display
	{
		displayIdx = SDL_GetWindowDisplayIndex( window );
		if( displayIdx < 0 ) // for some reason the display for the window couldn't be detected
		{
			displayIdx = 0;
		}
	}

	if( parms.fullScreen > SDL_GetNumVideoDisplays() )
	{
		common->Warning( "Can't set fullscreen mode to display number %i, because SDL2 only knows about %i displays!",
						 parms.fullScreen, SDL_GetNumVideoDisplays() );
		return -1;
	}

	if( parms.fullScreen != glConfig.isFullscreen )
	{
		// we have to switch to another display
		if( glConfig.isFullscreen )
		{
			// if we're already in fullscreen mode but want to switch to another monitor
			// we have to go to windowed mode first to move the window.. SDL-oddity.
			SDL_SetWindowFullscreen( window, SDL_FALSE );
		}
		// select display ; SDL_WINDOWPOS_UNDEFINED_DISPLAY() doesn't work.
		int x = SDL_WINDOWPOS_CENTERED_DISPLAY( displayIdx );
		// move window to the center of selected display
		SDL_SetWindowPosition( window, x, x );
	}
	return displayIdx;
}

static bool SetScreenParmsFullscreen( glimpParms_t parms )
{
	SDL_DisplayMode m = {0};
	int displayIdx = ScreenParmsHandleDisplayIndex( parms );
	if( displayIdx < 0 )
	{
		return false;
	}

	// get current mode of display the window should be full-screened on
	SDL_GetCurrentDisplayMode( displayIdx, &m );

	// change settings in that display mode according to parms
	// FIXME: check if refreshrate, width and height are supported?
	// m.refresh_rate = parms.displayHz;
	m.w = parms.width;
	m.h = parms.height;

	// set that displaymode
	if( SDL_SetWindowDisplayMode( window, &m ) < 0 )
	{
		common->Warning( "Couldn't set window mode for fullscreen, reason: %s", SDL_GetError() );
		return false;
	}

	// if we're currently not in fullscreen mode, we need to switch to fullscreen
	if( !( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ) )
	{
		if( SDL_SetWindowFullscreen( window, SDL_TRUE ) < 0 )
		{
			common->Warning( "Couldn't switch to fullscreen mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}
	return true;
}

static bool SetScreenParmsWindowed( glimpParms_t parms )
{
	SDL_SetWindowSize( window, parms.width, parms.height );
	SDL_SetWindowPosition( window, parms.x, parms.y );

	// if we're currently in fullscreen mode, we need to disable that
	if( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN )
	{
		if( SDL_SetWindowFullscreen( window, SDL_FALSE ) < 0 )
		{
			common->Warning( "Couldn't switch to windowed mode, reason: %s!", SDL_GetError() );
			return false;
		}
	}
	return true;
}


/*
===================
VKimp_SetScreenParms
===================
*/
bool VKimp_SetScreenParms( glimpParms_t parms )
{

	if( parms.fullScreen > 0 || parms.fullScreen == -2 )
	{
		if( !SetScreenParmsFullscreen( parms ) )
		{
			return false;
		}
	}
	else if( parms.fullScreen == 0 ) // windowed mode
	{
		if( !SetScreenParmsWindowed( parms ) )
		{
			return false;
		}
	}
	else
	{
		common->Warning( "VKimp_SetScreenParms: fullScreen -1 (borderless window for multiple displays) currently unsupported!" );
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.displayFrequency = parms.displayHz;
	glConfig.multisamples = parms.multiSamples;

	return true;
}

/*
===================
VKimp_Shutdown
===================
*/
void VKimp_Shutdown()
{
	common->Printf( "Shutting down Vulkan subsystem\n" );

	if( window )
	{
		SDL_DestroyWindow( window );
		window = nullptr;
	}

	if( SDL_WasInit( 0 ) )
	{
		SDL_Quit();
	}
}

/* Eric: Is this needed/used for Vulkan?
=================
VKimp_SetGamma
=================
*/
void VKimp_SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] )
{
#ifndef USE_VULKAN
	if( !window )
	{
		common->Warning( "VKimp_SetGamma called without window" );
		return;
	}

	if( SDL_SetWindowGammaRamp( window, red, green, blue ) )
	{
		common->Warning( "Couldn't set gamma ramp: %s", SDL_GetError() );
	}
#endif
}

/*
===================
VKimp_ExtensionPointer
===================
*/
/*
GLExtension_t VKimp_ExtensionPointer(const char *name) {
	assert(SDL_WasInit(SDL_INIT_VIDEO));

	return (GLExtension_t)SDL_GL_GetProcAddress(name);
}
*/

void VKimp_GrabInput( int flags )
{
	bool grab = flags & GRAB_ENABLE;

	if( grab && ( flags & GRAB_REENABLE ) )
	{
		grab = false;
	}

	if( flags & GRAB_SETSTATE )
	{
		grabbed = grab;
	}

	if( in_nograb.GetBool() )
	{
		grab = false;
	}

	if( !window )
	{
		common->Warning( "VKimp_GrabInput called without window" );
		return;
	}

	// DG: disabling the cursor is now done once in VKimp_Init() because it should always be disabled

	// DG: check for GRAB_ENABLE instead of GRAB_HIDECURSOR because we always wanna hide it
	SDL_SetRelativeMouseMode( flags & GRAB_ENABLE ? SDL_TRUE : SDL_FALSE );
	SDL_SetWindowGrab( window, grab ? SDL_TRUE : SDL_FALSE );

}