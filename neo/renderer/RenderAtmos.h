/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham
Copyright (C) 2026 Justin Marshall

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

#ifndef __RENDERATMOS_H__
#define __RENDERATMOS_H__

/*
===============================================================================

	Procedural sky

===============================================================================
*/

struct atmosFrame_t
{
	const idImage*		skyImage;			// NULL = gradient sky
	idVec3				skyTint;			// multiplies skyImage
	idVec3				zenithColor;
	idVec3				horizonColor;
	float				intensity;
	float				grade;

	float				cloudOpacity;		// 0 = no clouds
	float				cloudHeightMin;		// units above the eye
	float				cloudHeightMax;
	float				cloudScale;
	float				cloudCoverage;
	float				cloudDensity;
	float				cloudSeed;
	float				cloudVolume;
	int					cloudLayers;
	idVec2				cloudWind;
	idVec3				cloudColor;

	float				fogOpacity;			// 0 = no fog
	float				fogHeight;
	float				fogPower;
	idVec3				fogColor;
};

extern idCVar r_atmosDebug;

class idRenderWorldLocal;
struct viewDef_t;

const atmosFrame_t*	R_SetupAtmosForView( const idRenderWorldLocal* world, const viewDef_t* viewDef );
void				R_AtmosFlushCache();
int					R_AtmosPortalSkyMode( const idRenderWorldLocal* world );

#endif /* !__RENDERATMOS_H__ */
