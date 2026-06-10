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

#ifndef __CINEMATICBINK_H__
#define __CINEMATICBINK_H__

#undef nullptr

#include "libbinkdec/include/BinkDecoder.h"
#include "../sound/CinematicAudio.h"

/*
===============================================================================

	Bink cinematic

===============================================================================
*/

class idCinematicBink : public idCinematic
{
public:
public:
	idCinematicBink();
	virtual			~idCinematicBink();
	virtual bool	InitFromFile( const char* qpath, bool amilooping );
	virtual int		AnimationLength();
	virtual cinData_t	ImageForTime( int thisTime );
	virtual void	Close();
	virtual void	ResetTime( int time );
	virtual int		GetStartTime();
	virtual bool    IsPlaying() const;
	virtual void	ExportToTGA( bool skipExisting = true );
	virtual float	GetFrameRate() const;

private:
	BinkHandle				binkHandle;
	cinData_t				ImageForTimeBinkDec( int thisTime );
	bool					InitFromBinkDecFile( const char* qpath, bool looping );
	void					BinkDecReset();

	YUVbuffer				yuvBuffer;
	bool                    hasFrame;
	int						framePos;
	int						numFrames;
	idImage*				imgY;
	idImage*				imgCr;
	idImage*				imgCb;
	uint32_t				audioTracks;
	uint32_t				trackIndex;
	AudioInfo				binkInfo;

	int						CIN_WIDTH, CIN_HEIGHT;
	cinStatus_t				status;

	int						animationLength;
	int						startTime;
	float					frameRate;

	bool					looping;

	CinematicAudio*			cinematicAudio = NULL;
};

#endif /* !__CINEMATICBINK_H__ */
