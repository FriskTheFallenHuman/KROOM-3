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

#ifndef __RENDERDLLHOST_H__
#define __RENDERDLLHOST_H__

#include "RenderAPI.h"

extern idRenderHost*		renderHost;
extern renderHostCVars_t	renderHostCVars;

#define com_smp					( *renderHostCVars.com_smp )
#define com_engineHz			( *renderHostCVars.com_engineHz )
#define com_version				( *renderHostCVars.com_version )
#define com_developer			( *renderHostCVars.com_developer )
#define com_productionMode		( *renderHostCVars.com_productionMode )
#define s_noSound				( *renderHostCVars.s_noSound )
#define s_playCinematicAudio	( *renderHostCVars.s_playCinematicAudio )

#endif /* !__RENDERDLLHOST_H__ */
