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

#ifndef __DEVICEMANAGERAPI_H__
#define __DEVICEMANAGERAPI_H__

// sets the API specific window attributes and glConfig.driverType, returns the SDL window flag for the API
Uint32			DM_PrepareWindow( const vidParms_t& parms, int channelColorBits, int depthBits, int stencilBits );

// creates the API context for a new window, returns false if the window is unusable
bool			DM_CreateContext( SDL_Window* window );

// called once after a window was created successfully
void			DM_FinishInit( SDL_Window* window );

void			DM_SetMultisamples( int multiSamples );
const char*		DM_APIName();
void			DM_DestroyContext();
void			DM_ShutdownSDL( bool shutdownSDL );
void			DM_SwapBuffers( SDL_Window* window );
void			DM_TestSwapBuffers( SDL_Window* window );

#endif /* !__DEVICEMANAGERAPI_H__ */
