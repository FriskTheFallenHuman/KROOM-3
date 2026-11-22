/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2020 Robert Beckebans

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

#include "../RenderCommon.h"
#include "RenderBackend_GL.h"

/*
================================================================================================

Performance Events

================================================================================================
*/

/*
========================
PC_BeginNamedEvent
========================
*/
void PC_BeginNamedEvent( const char* szName, const idVec4& color )
{
	// only do this if the engine was started by RenderDoc or some similar tool
	if( glConfig.gremedyStringMarkerAvailable && glConfig.khronosDebugAvailable )
	{
		glPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION_ARB, 0, GLsizei( strlen( szName ) ), szName );
	}
}

/*
========================
PC_EndNamedEvent
========================
*/
void PC_EndNamedEvent()
{
	// only do this if the engine was started by RenderDoc or some similar tool
	if( glConfig.gremedyStringMarkerAvailable && glConfig.khronosDebugAvailable )
	{
		glPopDebugGroup();
	}
}

/*
========================
PC_LogMarker
========================
*/
void PC_LogMarker( const char* text )
{
	if( glConfig.gremedyStringMarkerAvailable )
	{
		glStringMarkerGREMEDY( strlen( text ), text );
	}
}

/*
========================
PC_BeginMainBlock
========================
*/
void PC_BeginMainBlock( renderLogMainBlock_t block )
{
#if defined(__APPLE__)
	// SRS - For OSX use elapsed time query for Apple OpenGL 4.1 using GL_TIME_ELAPSED vs GL_TIMESTAMP (which is not implemented on OSX)
	// SRS - OSX AMD drivers have a rendering bug (flashing colours) with an elasped time query when Shadow Mapping is on - turn off query for that case unless r_skipAMDWorkarounds is set
	if( !r_useShadowMapping.GetBool() || glConfig.vendor != VENDOR_AMD || r_skipAMDWorkarounds.GetBool() )
	{
		if( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 + 1 ] == 0 )
		{
			glGenQueries( 1, &glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 + 1 ] );
		}

		glBeginQuery( GL_TIME_ELAPSED_EXT, glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 + 1 ] );
	}
#else
	if( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 ] == 0 )
	{
		glCreateQueries( GL_TIMESTAMP, 2, &glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 ] );
	}

	glQueryCounter( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 + 0 ], GL_TIMESTAMP );
	glcontext.renderLogMainBlockTimeQueryIssued[ glcontext.frameParity ][ block * 2 + 0 ]++;
#endif
}

/*
========================
PC_EndMainBlock
========================
*/
void PC_EndMainBlock( renderLogMainBlock_t block )
{
#if defined(__APPLE__)
	// SRS - For OSX use elapsed time query for Apple OpenGL 4.1 using GL_TIME_ELAPSED vs GL_TIMESTAMP (which is not implemented on OSX)
	// SRS - OSX AMD drivers have a rendering bug (flashing colours) with an elasped time query when Shadow Mapping is on - turn off query for that case unless r_skipAMDWorkarounds is set
	if( !r_useShadowMapping.GetBool() || glConfig.vendor != VENDOR_AMD || r_skipAMDWorkarounds.GetBool() )
	{
		glEndQuery( GL_TIME_ELAPSED_EXT );
		glcontext.renderLogMainBlockTimeQueryIssued[ glcontext.frameParity ][ block * 2 + 1 ]++;
	}
#else
	glQueryCounter( glcontext.renderLogMainBlockTimeQueryIds[ glcontext.frameParity ][ block * 2 + 1 ], GL_TIMESTAMP );
	glcontext.renderLogMainBlockTimeQueryIssued[ glcontext.frameParity ][ block * 2 + 1 ]++;
#endif
}
