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

#include "precompiled.h"
#pragma hdrstop

#include "../sys_session_local.h"
#include "steam_voicechat.h"

#if defined(__APPLE__) && !defined(USE_OPENAL_SOFT_INCLUDES)
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
#endif

idCVar steam_voiceVolume( "steam_voiceVolume", "1", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE, "volume of the other players' voices, on top of the master volume", 0.0f, 2.0f );

static const int VOICE_NUM_BUFFERS = 16;			// about half a second of audio, a chunk that finds none free is dropped to keep the delay bounded
static const int VOICE_PREBUFFER = 2;			// chunks queued before playback starts, absorbs the jitter of the network
static const uint32	VOICE_INITIAL_PCM_SIZE = 20 * 1024;
static const uint32	VOICE_MAX_PCM_SIZE = 256 * 1024;
static const int PUSH_TO_TALK_RELEASE_MS = 250;		// keeps recording a moment after the key is released, the end of the last word would be cut off

/*
================================================
idSteamVoicePlayer

Streams the decoded voice of one remote talker. The audio is mono 16 bit, played as a 2D source.
================================================
*/
class idSteamVoicePlayer
{
public:
	idSteamVoicePlayer() : source( 0 ), numFree( 0 ), initialized( false ), failed( false ) {}
	~idSteamVoicePlayer() {}

	void	QueuePCM( const short* samples, int numSamples, int rate, float gain );
	void	SetGain( float gain );
	void	Flush();
	void	Shutdown();

private:
	bool	Init( float gain );
	void	Reclaim();

	ALuint	source;
	ALuint	buffers[ VOICE_NUM_BUFFERS ];
	ALuint	freeBuffers[ VOICE_NUM_BUFFERS ];
	int		numFree;
	bool	initialized;
	bool	failed;			// no OpenAL context or no source, do not retry on every packet
};

/*
========================
idSteamVoicePlayer::Init
========================
*/
bool idSteamVoicePlayer::Init( float gain )
{
	if( initialized )
	{
		return true;
	}
	if( failed )
	{
		return false;
	}

	if( alcGetCurrentContext() == NULL )
	{
		idLib::Warning( "STEAM: voice chat playback needs an OpenAL context" );
		failed = true;
		return false;
	}

	alGetError();
	alGenSources( 1, &source );
	if( alGetError() != AL_NO_ERROR )
	{
		idLib::Warning( "STEAM: voice chat could not create an OpenAL source" );
		failed = true;
		return false;
	}

	alGenBuffers( VOICE_NUM_BUFFERS, buffers );
	if( alGetError() != AL_NO_ERROR )
	{
		alDeleteSources( 1, &source );
		source = 0;
		idLib::Warning( "STEAM: voice chat could not create OpenAL buffers" );
		failed = true;
		return false;
	}

	alSource3f( source, AL_POSITION, 0.0f, 0.0f, 0.0f );
	alSourcei( source, AL_SOURCE_RELATIVE, AL_TRUE );
	alSourcef( source, AL_ROLLOFF_FACTOR, 0.0f );
	alSourcef( source, AL_GAIN, gain );

	for( int i = 0; i < VOICE_NUM_BUFFERS; i++ )
	{
		freeBuffers[i] = buffers[i];
	}
	numFree = VOICE_NUM_BUFFERS;

	initialized = true;
	return true;
}

/*
========================
idSteamVoicePlayer::Reclaim
========================
*/
void idSteamVoicePlayer::Reclaim()
{
	ALint processed = 0;
	alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	while( processed-- > 0 && numFree < VOICE_NUM_BUFFERS )
	{
		ALuint buffer = 0;
		alSourceUnqueueBuffers( source, 1, &buffer );
		freeBuffers[ numFree++ ] = buffer;
	}
}

/*
========================
idSteamVoicePlayer::QueuePCM
========================
*/
void idSteamVoicePlayer::QueuePCM( const short* samples, int numSamples, int rate, float gain )
{
	if( numSamples <= 0 || !Init( gain ) )
	{
		return;
	}

	Reclaim();

	if( numFree == 0 )
	{
		STEAM_VERBOSE_PRINT( "STEAM: voice playback is behind, dropped a chunk\n" );
		return;
	}

	const ALuint buffer = freeBuffers[ --numFree ];
	alBufferData( buffer, AL_FORMAT_MONO16, samples, numSamples * ( int )sizeof( short ), rate );
	alSourceQueueBuffers( source, 1, &buffer );
	if( alGetError() != AL_NO_ERROR )
	{
		freeBuffers[ numFree++ ] = buffer;
		return;
	}

	ALint state = 0;
	ALint queued = 0;
	alGetSourcei( source, AL_SOURCE_STATE, &state );
	alGetSourcei( source, AL_BUFFERS_QUEUED, &queued );
	if( state != AL_PLAYING && queued >= VOICE_PREBUFFER )
	{
		alSourcePlay( source );
	}
}

/*
========================
idSteamVoicePlayer::SetGain
========================
*/
void idSteamVoicePlayer::SetGain( float gain )
{
	if( initialized )
	{
		alSourcef( source, AL_GAIN, gain );
	}
}

/*
========================
idSteamVoicePlayer::Flush
========================
*/
void idSteamVoicePlayer::Flush()
{
	if( !initialized )
	{
		return;
	}

	alSourceStop( source );
	alSourcei( source, AL_BUFFER, 0 );		// detaches every queued buffer
	for( int i = 0; i < VOICE_NUM_BUFFERS; i++ )
	{
		freeBuffers[i] = buffers[i];
	}
	numFree = VOICE_NUM_BUFFERS;
}

/*
========================
idSteamVoicePlayer::Shutdown
========================
*/
void idSteamVoicePlayer::Shutdown()
{
	if( initialized )
	{
		alSourceStop( source );
		alSourcei( source, AL_BUFFER, 0 );
		alDeleteSources( 1, &source );
		alDeleteBuffers( VOICE_NUM_BUFFERS, buffers );
	}
	source = 0;
	numFree = 0;
	initialized = false;
	failed = false;
}

/*
================================================
idVoiceChatMgrSteam::remoteVoice_t
================================================
*/
struct idVoiceChatMgrSteam::remoteVoice_t
{
	const lobbyUser_t*	user;
	idSteamVoicePlayer	player;
};

/*
========================
GetVoiceGain
========================
*/
static float GetVoiceGain()
{
	return idMath::ClampFloat( 0.0f, 2.0f, steam_voiceVolume.GetFloat() );
}

/*
========================
idVoiceChatMgrSteam::idVoiceChatMgrSteam
========================
*/
idVoiceChatMgrSteam::idVoiceChatMgrSteam() :
	decompressBuffer( NULL ),
	decompressBufferSize( 0 ),
	sampleRate( 24000 ),
	appliedGain( 1.0f ),
	recording( false ),
	warnedCaptureOverflow( false ),
	lastTalkKeyTime( 0 )
{
}

/*
========================
idVoiceChatMgrSteam::~idVoiceChatMgrSteam
========================
*/
idVoiceChatMgrSteam::~idVoiceChatMgrSteam()
{
	remoteVoices.DeleteContents( true );
	if( decompressBuffer != NULL )
	{
		Mem_Free( decompressBuffer );
		decompressBuffer = NULL;
	}
}

/*
========================
idVoiceChatMgrSteam::Init
========================
*/
void idVoiceChatMgrSteam::Init( void* pXAudio2 )
{
	idVoiceChatMgr::Init( pXAudio2 );

	ISteamUser* user = SteamUser();
	if( user != NULL )
	{
		sampleRate = user->GetVoiceOptimalSampleRate();
	}
	STEAM_VERBOSE_PRINT( "STEAM: voice chat decodes at %u Hz\n", sampleRate );

	decompressBufferSize = VOICE_INITIAL_PCM_SIZE;
	decompressBuffer = ( byte* )Mem_Alloc( decompressBufferSize, TAG_SYSTEM );
}

/*
========================
idVoiceChatMgrSteam::Shutdown
========================
*/
void idVoiceChatMgrSteam::Shutdown()
{
	if( recording && Steam_IsRunning() && SteamUser() != NULL )
	{
		SteamUser()->StopVoiceRecording();
	}
	recording = false;

	for( int i = 0; i < remoteVoices.Num(); i++ )
	{
		remoteVoices[i]->player.Shutdown();
	}
	remoteVoices.DeleteContents( true );

	idVoiceChatMgr::Shutdown();
}

/*
========================
idVoiceChatMgrSteam::FindCaptureTalker
========================
*/
int idVoiceChatMgrSteam::FindCaptureTalker() const
{
	for( int i = 0; i < talkers.Num(); i++ )
	{
		if( talkers[i].IsLocal() && talkers[i].registeredSuccess )
		{
			return i;
		}
	}
	return -1;
}

/*
========================
idVoiceChatMgrSteam::FindRemoteVoice
========================
*/
idVoiceChatMgrSteam::remoteVoice_t* idVoiceChatMgrSteam::FindRemoteVoice( const lobbyUser_t* user ) const
{
	for( int i = 0; i < remoteVoices.Num(); i++ )
	{
		if( remoteVoices[i]->user == user )
		{
			return remoteVoices[i];
		}
	}
	return NULL;
}

/*
========================
idVoiceChatMgrSteam::RegisterTalkerInternal
========================
*/
bool idVoiceChatMgrSteam::RegisterTalkerInternal( int index )
{
	const talker_t& talker = talkers[ index ];

	if( talker.IsLocal() )
	{
		return SteamUser() != NULL;
	}

	if( FindRemoteVoice( talker.user ) == NULL )
	{
		remoteVoice_t* voice = new( TAG_SYSTEM ) remoteVoice_t;
		voice->user = talker.user;
		remoteVoices.Append( voice );
	}
	return true;
}

/*
========================
idVoiceChatMgrSteam::UnregisterTalkerInternal
========================
*/
void idVoiceChatMgrSteam::UnregisterTalkerInternal( int index )
{
	const talker_t& talker = talkers[ index ];

	if( talker.IsLocal() )
	{
		return;
	}

	for( int i = 0; i < remoteVoices.Num(); i++ )
	{
		if( remoteVoices[i]->user == talker.user )
		{
			remoteVoices[i]->player.Shutdown();
			delete remoteVoices[i];
			remoteVoices.RemoveIndex( i );
			return;
		}
	}
}

/*
========================
idVoiceChatMgrSteam::UpdateCapture
========================
*/
void idVoiceChatMgrSteam::UpdateCapture()
{
	ISteamUser* user = SteamUser();
	if( user == NULL )
	{
		recording = false;
		return;
	}

	const int mode = voice_mode.GetInteger();
	bool wanted = mode != VOICE_MODE_DISABLED && FindCaptureTalker() != -1;

	if( wanted && mode == VOICE_MODE_PUSH_TO_TALK )
	{
		const int now = Sys_Milliseconds();
		if( usercmdGen != NULL && usercmdGen->ButtonState( UB_TALK ) > 0 )
		{
			lastTalkKeyTime = now;
		}
		wanted = lastTalkKeyTime != 0 && ( now - lastTalkKeyTime ) < PUSH_TO_TALK_RELEASE_MS;
	}
	else
	{
		lastTalkKeyTime = 0;
	}

	if( wanted && !recording )
	{
		user->StartVoiceRecording();
		recording = true;
	}
	else if( !wanted && recording )
	{
		user->StopVoiceRecording();
		recording = false;
	}
}

/*
========================
idVoiceChatMgrSteam::Pump
========================
*/
void idVoiceChatMgrSteam::Pump()
{
	UpdateCapture();

	const float gain = GetVoiceGain();
	if( gain != appliedGain )
	{
		appliedGain = gain;
		for( int i = 0; i < remoteVoices.Num(); i++ )
		{
			remoteVoices[i]->player.SetGain( gain );
		}
	}
}

/*
========================
idVoiceChatMgrSteam::FlushBuffers
========================
*/
void idVoiceChatMgrSteam::FlushBuffers()
{
	for( int i = 0; i < remoteVoices.Num(); i++ )
	{
		remoteVoices[i]->player.Flush();
	}
}

/*
========================
idVoiceChatMgrSteam::TalkerHasData
========================
*/
bool idVoiceChatMgrSteam::TalkerHasData( int talkerIndex )
{
	if( !recording || talkerIndex != FindCaptureTalker() )
	{
		return false;
	}

	uint32 available = 0;
	return SteamUser()->GetAvailableVoice( &available ) == k_EVoiceResultOK && available > 0;
}

/*
========================
idVoiceChatMgrSteam::GetLocalChatDataInternal
========================
*/
bool idVoiceChatMgrSteam::GetLocalChatDataInternal( int talkerIndex, byte* data, int& dataSize )
{
	if( !recording || talkerIndex != FindCaptureTalker() || dataSize <= 0 )
	{
		return false;
	}

	uint32 written = 0;
	const EVoiceResult result = SteamUser()->GetVoice( true, data, ( uint32 )dataSize, &written );

	if( result == k_EVoiceResultBufferTooSmall && !warnedCaptureOverflow )
	{
		// the data stays in the buffer, nothing is lost but nothing is sent either
		warnedCaptureOverflow = true;
		idLib::Warning( "STEAM: captured voice does not fit a %d byte packet", dataSize );
	}

	if( result != k_EVoiceResultOK || written == 0 )
	{
		return false;
	}

	dataSize = ( int )written;
	return true;
}

/*
========================
idVoiceChatMgrSteam::Decompress
Decodes into decompressBuffer, grows it when Steam asks for more.
========================
*/
bool idVoiceChatMgrSteam::Decompress( const byte* data, int dataSize, uint32& numBytes )
{
	ISteamUser* user = SteamUser();
	if( user == NULL || decompressBuffer == NULL )
	{
		return false;
	}

	numBytes = 0;
	EVoiceResult result = user->DecompressVoice( data, ( uint32 )dataSize, decompressBuffer, decompressBufferSize, &numBytes, sampleRate );

	if( result == k_EVoiceResultBufferTooSmall && numBytes > decompressBufferSize && numBytes <= VOICE_MAX_PCM_SIZE )
	{
		Mem_Free( decompressBuffer );
		decompressBufferSize = numBytes;
		decompressBuffer = ( byte* )Mem_Alloc( decompressBufferSize, TAG_SYSTEM );

		numBytes = 0;
		result = user->DecompressVoice( data, ( uint32 )dataSize, decompressBuffer, decompressBufferSize, &numBytes, sampleRate );
	}

	return result == k_EVoiceResultOK && numBytes >= sizeof( short );
}

/*
========================
idVoiceChatMgrSteam::SubmitIncomingChatDataInternal
========================
*/
void idVoiceChatMgrSteam::SubmitIncomingChatDataInternal( int talkerIndex, const byte* data, int dataSize )
{
	if( voice_mode.GetInteger() == VOICE_MODE_DISABLED || dataSize <= 0 )
	{
		return;
	}

	remoteVoice_t* voice = FindRemoteVoice( talkers[ talkerIndex ].user );
	if( voice == NULL )
	{
		return;
	}

	uint32 numBytes = 0;
	if( !Decompress( data, dataSize, numBytes ) )
	{
		return;
	}

	voice->player.QueuePCM( ( const short* )decompressBuffer, ( int )( numBytes / sizeof( short ) ), ( int )sampleRate, GetVoiceGain() );
}