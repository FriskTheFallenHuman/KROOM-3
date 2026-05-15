/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2015 Robert Beckebans

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

#ifndef __GAME_TONEMAPCONTROLLER_H__
#define __GAME_TONEMAPCONTROLLER_H__

/*
===============================================================================

  Tonemap controller.

===============================================================================
*/

struct tonemapState_t
{
	tonemapState_t()
	{
		preset = 0;
		exposure = 0.5f;
		saturation = 1.0f;
		contrast = 1.0f;
		hdrKey = 0.015f;
	}

	int preset;
	float exposure;
	float saturation;
	float contrast;
	float hdrKey;
};

class idTonemapController : public idEntity
{
	friend class idGameLocal;

public:
	CLASS_PROTOTYPE( idTonemapController );

	idTonemapController();
	~idTonemapController();

	void			Spawn();
	void			Save( idSaveGame* savefile ) const;					// archives object for save game file
	void			Restore( idRestoreGame* savefile );					// unarchives object from save game file
	virtual void	Think();
	virtual void	WriteToSnapshot( idBitMsg& msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsg& msg );
	virtual void	ClientPredictionThink();
	virtual bool	ClientReceiveEvent( int event, int time, const idBitMsg& msg );

	static bool		IsControllerActive()
	{
		return ( tonemapBlendAlpha < 1.0f );
	}

private:
	static tonemapState_t	tonemapCurrent;	// the current tonemap settings for the player's view, used for syncing with the backend
	static tonemapState_t	tonemapTarget;	// the target tonemap settings that the current settings will blend towards
	static float	tonemapBlendAlpha;
	static float	tonemapBlendSpeed;

	float       activateRadius;
	float       blendTime;
	float       blendAlpha;     // 0 = inactive, 1 = fully active

	// Target params
	int         preset;
	float       exposure;
	float       saturation;
	float       contrast;
	float       hdrKey;

	// Previous (blended-from) params
	int         prevPreset;
	float       prevExposure;
	float       prevSaturation;
	float       prevContrast;
	float       prevKey;

	bool        active;

private:
	void        Event_Enable();
	void        Event_Disable();
};

#endif /* !__GAME_TONEMAPCONTROLLER_H__ */
