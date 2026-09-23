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

#include "precompiled.h"
#pragma hdrstop

#include "AL_CinematicAudio.h"
#include "../snd_local.h"

#if defined(USE_FFMPEG)
extern "C"
{
#define __STDC_CONSTANT_MACROS
#include <libavcodec/avcodec.h>
}
#endif

#include <BinkDecoder.h>

extern idCVar s_volume_dB;

static bool isBink = false;

CinematicAudio_OpenAL::CinematicAudio_OpenAL():
	av_rate_cin( 0 ),
	av_sample_cin( 0 ),
	offset( 0 ),
	trigger( false )
{
	alGenSources( 1, &alMusicSourceVoicecin );

	alSource3i( alMusicSourceVoicecin, AL_POSITION, 0, 0, 0 );
	alSourcei( alMusicSourceVoicecin, AL_SOURCE_RELATIVE, AL_TRUE );
	alSourcei( alMusicSourceVoicecin, AL_ROLLOFF_FACTOR, 0 );
	alListenerf( AL_GAIN, DBtoLinear( s_volume_dB.GetFloat() ) ); //GK: Set the sound volume the same that is used in DOOM 3
	alGenBuffers( NUM_BUFFERS, &alMusicBuffercin[0] );
}

CinematicAudio_OpenAL::CinematicAudio_OpenAL( bool bBinkFile ):
	av_rate_cin( 0 ),
	av_sample_cin( 0 ),
	offset( 0 ),
	trigger( false )
{
	isBink = bBinkFile;

	alGenSources( 1, &alMusicSourceVoicecin );

	alSource3i( alMusicSourceVoicecin, AL_POSITION, 0, 0, 0 );
	alSourcei( alMusicSourceVoicecin, AL_SOURCE_RELATIVE, AL_TRUE );
	alSourcei( alMusicSourceVoicecin, AL_ROLLOFF_FACTOR, 0 );
	alListenerf( AL_GAIN, DBtoLinear( s_volume_dB.GetFloat() ) ); //GK: Set the sound volume the same that is used in DOOM 3
	alGenBuffers( NUM_BUFFERS, &alMusicBuffercin[0] );
}

void CinematicAudio_OpenAL::InitAudio( void* audioContext )
{
	if( isBink )
	{
		AudioInfo* binkInfo = ( AudioInfo* )audioContext;
		av_rate_cin = binkInfo->sampleRate;
		av_sample_cin = binkInfo->nChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	}
	else
	{
#if defined(USE_FFMPEG)
		AVCodecContext* dec_ctx2 = ( AVCodecContext* )audioContext;
		av_rate_cin = dec_ctx2->sample_rate;

		switch( dec_ctx2->sample_fmt )
		{
			case AV_SAMPLE_FMT_U8:
			case AV_SAMPLE_FMT_U8P:
			{
#if	LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
				av_sample_cin = dec_ctx2->ch_layout.nb_channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
#else
				av_sample_cin = dec_ctx2->channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
#endif
				break;
			}
			case AV_SAMPLE_FMT_S16:
			case AV_SAMPLE_FMT_S16P:
			{
#if	LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
				av_sample_cin = dec_ctx2->ch_layout.nb_channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
#else
				av_sample_cin = dec_ctx2->channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
#endif
				break;
			}
			case AV_SAMPLE_FMT_FLT:
			case AV_SAMPLE_FMT_FLTP:
			{
#if	LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59,37,100)
				av_sample_cin = dec_ctx2->ch_layout.nb_channels == 2 ? AL_FORMAT_STEREO_FLOAT32 : AL_FORMAT_MONO_FLOAT32;
#else
				av_sample_cin = dec_ctx2->channels == 2 ? AL_FORMAT_STEREO_FLOAT32 : AL_FORMAT_MONO_FLOAT32;
#endif
				break;
			}
			default:
			{
				common->Warning( "Unknown or incompatible cinematic audio format for OpenAL, sample_fmt = %d\n", dec_ctx2->sample_fmt );
				return;
			}
		}
#endif
	}

	alSourceRewind( alMusicSourceVoicecin );
	alSourcei( alMusicSourceVoicecin, AL_BUFFER, 0 );
	offset = 0;
	trigger = false;
}

void CinematicAudio_OpenAL::PlayAudio( uint8_t* data, int size )
{
	ALint processed, state;

	alGetSourcei( alMusicSourceVoicecin, AL_SOURCE_STATE, &state );
	alGetSourcei( alMusicSourceVoicecin, AL_BUFFERS_PROCESSED, &processed );
	//common->Printf( "AL_CinematicAudio: processed = %2d, bufids = %2d, tBuffers = %2d, state = %d\n", processed, bufids.size(), tBuffer.size(), state );

	if( trigger )
	{
		ALuint bufid;

		// SRS - Unqueue all processed alBuffers and place them on the free bufids queue
		while( processed > 0 )
		{
			alSourceUnqueueBuffers( alMusicSourceVoicecin, 1, &bufid );
			bufids.push( bufid );
			processed--;
		}

		tBuffer.push( data );
		sizes.push( size );
		while( !tBuffer.empty() )
		{
			// SRS - If we have an audio buffer ready to play and a free bufid, then queue it up
			if( !bufids.empty() )
			{
				uint8_t* tempdata = tBuffer.front();
				tBuffer.pop();
				int tempSize = sizes.front();
				sizes.pop();
				if( tempdata )
				{
					bufid = bufids.front();
					bufids.pop();
					alBufferData( bufid, av_sample_cin, tempdata, tempSize, av_rate_cin );

					// SRS - We must free the audio buffer once it has been copied into an alBuffer
					if( isBink )
					{
						Mem_Free( tempdata );
					}
					else
					{
#if defined(USE_FFMPEG)
						av_freep( &tempdata );
#endif
					}

					CheckALErrors();

					alSourceQueueBuffers( alMusicSourceVoicecin, 1, &bufid );
					if( CheckALErrors() != AL_NO_ERROR )
					{
						common->Warning( "CinematicAudio_OpenAL::PlayAudio: error queueing OpenAL hardware buffers" );
						return;
					}
				}
			}
			// SRS - If there are no available bufids remaining, break and continue playing
			else
			{
				break;
			}
		}
	}
	else
	{
		alBufferData( alMusicBuffercin[offset], av_sample_cin, data, size, av_rate_cin );

		// SRS - We must free the audio buffer once it has been copied into an alBuffer
		if( isBink )
		{
			Mem_Free( data );
			data = NULL;
		}
		else
		{
#if defined(USE_FFMPEG)
			av_freep( &data );
#endif
		}

		offset++;

		// SRS - Initiate playback trigger once we have MIN_BUFFERS filled: limit startup latency
		if( offset == MIN_BUFFERS )
		{
			CheckALErrors();
			alSourceQueueBuffers( alMusicSourceVoicecin, MIN_BUFFERS, &alMusicBuffercin[0] );
			if( CheckALErrors() != AL_NO_ERROR )
			{
				common->Warning( "CinematicAudio_OpenAL::PlayAudio: error queueing OpenAL hardware buffers" );
				return;
			}
			// SRS - Prepare additional free buffers to handle variable packet rate codecs (e.g. webm vorbis)
			for( int i = MIN_BUFFERS; i < NUM_BUFFERS; i++ )
			{
				bufids.push( alMusicBuffercin[ i ] );
			}
			trigger = true;
		}
	}

	if( trigger )
	{
		if( state != AL_PLAYING )
		{
			ALint queued;
			alGetSourcei( alMusicSourceVoicecin, AL_BUFFERS_QUEUED, &queued );
			if( queued == 0 )
			{
				return;
			}
			CheckALErrors();
			alSourcePlay( alMusicSourceVoicecin );
			if( CheckALErrors() != AL_NO_ERROR )
			{
				common->Warning( "CinematicAudio_OpenAL::PlayAudio: error playing OpenAL streaming source" );
				return;
			}
		}
	}
}

void CinematicAudio_OpenAL::ResetAudio()
{
	if( alIsSource( alMusicSourceVoicecin ) )
	{
		alSourceRewind( alMusicSourceVoicecin );
		alSourcei( alMusicSourceVoicecin, AL_BUFFER, 0 );
	}

	while( !tBuffer.empty() )
	{
		uint8_t* tempdata = tBuffer.front();
		tBuffer.pop();
		sizes.pop();
		if( tempdata )
		{
			// SRS - We must free any audio buffers that have not been copied into an alBuffer
			if( isBink )
			{
				Mem_Free( tempdata );
			}
			else
			{
#if defined(USE_FFMPEG)
				av_freep( &tempdata );
#endif
			}
		}
	}

	while( !bufids.empty() )
	{
		bufids.pop();
	}

	offset = 0;
	trigger = false;
}

void CinematicAudio_OpenAL::ShutdownAudio()
{
	if( alIsSource( alMusicSourceVoicecin ) )
	{
		alSourceStop( alMusicSourceVoicecin );
		alSourcei( alMusicSourceVoicecin, AL_BUFFER, 0 );

		CheckALErrors();
		alDeleteSources( 1, &alMusicSourceVoicecin );
		if( CheckALErrors() == AL_NO_ERROR )
		{
			alMusicSourceVoicecin = 0;
		}
	}

	for( int i = 0; i < NUM_BUFFERS; i++ )
	{
		if( alIsBuffer( alMusicBuffercin[i] ) )
		{
			CheckALErrors();
			alDeleteBuffers( 1, &alMusicBuffercin[i] );
			if( CheckALErrors() == AL_NO_ERROR )
			{
				alMusicBuffercin[i] = 0;
			}
		}
	}

	while( !tBuffer.empty() )
	{
		uint8_t* tempdata = tBuffer.front();
		tBuffer.pop();
		sizes.pop();
		if( tempdata )
		{
			// SRS - We must free any audio buffers that have not been copied into an alBuffer
			if( isBink )
			{
				Mem_Free( tempdata );
			}
			else
			{
#if defined(USE_FFMPEG)
				av_freep( &tempdata );
#endif
			}
		}
	}

	while( !bufids.empty() )
	{
		bufids.pop();
	}
}
