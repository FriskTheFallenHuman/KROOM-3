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

#ifndef __DECLATMOS_H__
#define __DECLATMOS_H__

class idMaterial;

/*
===============================================================================

	idDeclAtmos

===============================================================================
*/

enum atmosPortalSky_t
{
	ATMOS_PORTALSKY_BEHIND = 0,		// sky dome/clouds are rendered inside the portal sky pass, behind the sky room
	ATMOS_PORTALSKY_REPLACE,		// portal sky pass is skipped, the atmosphere replaces it
	ATMOS_PORTALSKY_OFF				// no atmosphere while a portal sky is active
};

struct atmosClouds_t
{
	bool		enabled;
	float		heightMin;		// units above the eye
	float		heightMax;
	float		scale;
	float		coverage;
	float		density;
	float		opacity;
	float		seed;
	bool		autoColor;
	idVec3		color;
	idVec2		wind;
	float		volume;
};

struct atmosFog_t
{
	bool		enabled;
	bool		autoColor;
	idVec3		color;
	float		opacity;
	float		height;
	float		power;
};

struct atmosParms_t
{
	const idMaterial*	skyMaterial;
	idVec3				skyTint;
	idVec3				zenithColor;
	idVec3				horizonColor;
	float				intensity;
	float				grade;
	atmosPortalSky_t	portalSky;
	atmosClouds_t		clouds;
	atmosFog_t			fog;

	void				SetDefaults();
};

class idDeclAtmos : public idDecl
{
public:
	virtual size_t			Size() const;
	virtual bool			SetDefaultText();
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	const atmosParms_t&		GetParms() const
	{
		return parms;
	}

private:
	atmosParms_t			parms;
};

#endif /* !__DECLATMOS_H__ */
