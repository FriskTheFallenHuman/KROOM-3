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
#ifndef __STEAM_VOICECHAT_H__
#define __STEAM_VOICECHAT_H__

#include "../sys_voicechat.h"
#include "steam_local.h"

/*
================================================

idVoiceChatMgrSteam

================================================
*/
class idVoiceChatMgrSteam final : public idVoiceChatMgr
{
public:
	idVoiceChatMgrSteam();
	~idVoiceChatMgrSteam() override;

	idVoiceChatMgrSteam( const idVoiceChatMgrSteam& ) = delete;
	idVoiceChatMgrSteam& operator=( const idVoiceChatMgrSteam& ) = delete;

	void	Init( void* pXAudio2 ) override;
	void	Shutdown() override;
	void	Pump() override;
	void	FlushBuffers() override;

	bool	GetLocalChatDataInternal( int talkerIndex, byte* data, int& dataSize ) override;
	void	SubmitIncomingChatDataInternal( int talkerIndex, const byte* data, int dataSize ) override;
	bool	TalkerHasData( int talkerIndex ) override;

	bool	RegisterTalkerInternal( int index ) override;
	void	UnregisterTalkerInternal( int index ) override;

private:
	struct remoteVoice_t;

	int				FindCaptureTalker() const;
	remoteVoice_t* 	FindRemoteVoice( const lobbyUser_t* user ) const;
	void			UpdateCapture();
	bool			Decompress( const byte* data, int dataSize, uint32& numBytes );

	idList< remoteVoice_t* >	remoteVoices;		// one per registered remote talker

	byte* 			decompressBuffer;				// PCM of the packet that is being played
	uint32			decompressBufferSize;
	uint32			sampleRate;						// ISteamUser::GetVoiceOptimalSampleRate()
	float			appliedGain;
	bool			recording;
	bool			warnedCaptureOverflow;
	int				lastTalkKeyTime;				// last time the voice key was seen down
};

#endif // __STEAM_VOICECHAT_H__
