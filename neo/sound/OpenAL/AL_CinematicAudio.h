/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2021 George Kalmpokis

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

#ifndef __AL_CINEMATICAUDIO_H__
#define __AL_CINEMATICAUDIO_H__

#include <precompiled.h>
#include "../CinematicAudio.h"
// SRS - Added check on OSX for OpenAL Soft headers vs macOS SDK headers
#if defined(__APPLE__) && !defined(USE_OPENAL_SOFT_INCLUDES)
	#include <OpenAL/al.h>
#else
	#include <AL/al.h>
#endif

#include <queue>
#define MIN_BUFFERS 4					// SRS - Minimum buffers to fill before triggering playback
#define NUM_BUFFERS 16					// SRS - Total buffers available to support variable rate codecs

class CinematicAudio_OpenAL: public CinematicAudio
{
public:
	CinematicAudio_OpenAL();
	CinematicAudio_OpenAL( bool bBinkFile );
	void InitAudio( void* audioContext );
	void PlayAudio( uint8_t* data, int size );
	void ResetAudio();
	void ShutdownAudio();
private:
	ALuint		alMusicSourceVoicecin;
	ALuint		alMusicBuffercin[NUM_BUFFERS];
	ALenum		av_sample_cin;
	int			av_rate_cin;
	int			offset;
	bool		trigger;

	//GK: Unlike XAudio2 which can accept buffer until the end of this world.
	//	  OpenAL can accept buffers as long as there are freely available buffers.
	//	  So, what happens if there are no freely available buffers but we still geting audio frames ? Loss of data.
	//	  That why now I am using two queues in order to store the frames (and their sizes) and when we have available buffers,
	//	  then start popping those frames instead of the current, so we don't lose any audio frames and the sound doesn't crack anymore.
	std::queue<uint8_t*>	tBuffer;
	std::queue<int>			sizes;
	std::queue<ALuint>		bufids;		// SRS - Added queue of free alBuffer ids to handle variable rate codecs
};

#endif /* !__AL_CINEMATICAUDIO_H__ */
