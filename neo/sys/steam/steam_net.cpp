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
#include "steam_local.h"

idCVar steam_netAcceptAny( "steam_netAcceptAny", "0", CVAR_SYSTEM | CVAR_BOOL, "accept packet sessions from any Steam user instead of lobby members only (testing)" );

/*
================================================================================================

	Datagram transport between peers addressed by Steam id (ISteamNetworkingMessages).

================================================================================================
*/

static const int NET_CHANNEL = 0;
static const int NET_SEND_FLAGS = k_nSteamNetworkingSend_UnreliableNoNagle | k_nSteamNetworkingSend_AutoRestartBrokenSession;
static const int MAX_PENDING_REQUESTS = 16;
static const int PENDING_REQUEST_TIMEOUT_MS	= 10 * 1000;	// the lobby member list of the host can be a moment behind the packets of a new member
static const int MAX_TRACKED_PEERS = 64;
static const int IDLE_SESSION_TIMEOUT_MS = 30 * 1000;
static const int CLEANUP_INTERVAL_MS = 5 * 1000;

/*
================================================
idSteamNet
================================================
*/
class idSteamNet
{
public:
	idSteamNet() : numPending( 0 ), numTracked( 0 ), lastCleanup( 0 ), numFailedSends( 0 ) {}

	void	Touch( uint64 steamId );
	void	AcceptSession( uint64 steamId );
	void	Update();
	void	CloseAll();
	void	NoteSendFailure( uint64 steamId, EResult result );

private:
	STEAM_CALLBACK( idSteamNet, OnSessionRequest, SteamNetworkingMessagesSessionRequest_t );
	STEAM_CALLBACK( idSteamNet, OnSessionFailed, SteamNetworkingMessagesSessionFailed_t );

	struct pendingRequest_t
	{
		uint64	steamId;
		int		time;
	};
	struct trackedPeer_t
	{
		uint64	steamId;
		int		lastUse;
	};

	pendingRequest_t	pending[ MAX_PENDING_REQUESTS ];
	int					numPending;
	trackedPeer_t		tracked[ MAX_TRACKED_PEERS ];
	int					numTracked;
	int					lastCleanup;
	int					numFailedSends;
};

static idSteamNet* steamNet = NULL;

/*
========================
EnsureNet
========================
*/
static idSteamNet* EnsureNet()
{
	if( steamNet == NULL )
	{
		if( !Steam_IsRunning() || SteamNetworkingMessages() == NULL )
		{
			return NULL;
		}
		steamNet = new( TAG_NETWORKING ) idSteamNet();
	}
	return steamNet;
}

/*
========================
idSteamNet::Touch
========================
*/
void idSteamNet::Touch( uint64 steamId )
{
	const int now = Sys_Milliseconds();

	for( int i = 0; i < numTracked; i++ )
	{
		if( tracked[i].steamId == steamId )
		{
			tracked[i].lastUse = now;
			return;
		}
	}

	if( numTracked == MAX_TRACKED_PEERS )
	{
		// evict the user we did not talk to for the longest time
		int oldest = 0;
		for( int i = 1; i < numTracked; i++ )
		{
			if( tracked[i].lastUse < tracked[oldest].lastUse )
			{
				oldest = i;
			}
		}
		SteamNetworkingIdentity identity;
		identity.SetSteamID64( tracked[oldest].steamId );
		SteamNetworkingMessages()->CloseSessionWithUser( identity );
		tracked[oldest] = tracked[ --numTracked ];
	}

	tracked[ numTracked ].steamId = steamId;
	tracked[ numTracked ].lastUse = now;
	numTracked++;
}

/*
========================
idSteamNet::AcceptSession
========================
*/
void idSteamNet::AcceptSession( uint64 steamId )
{
	SteamNetworkingIdentity identity;
	identity.SetSteamID64( steamId );

	if( SteamNetworkingMessages()->AcceptSessionWithUser( identity ) )
	{
		STEAM_VERBOSE_PRINT( "STEAM: accepted packet session with %llu\n", ( unsigned long long )steamId );
		Touch( steamId );
	}
}

/*
========================
idSteamNet::OnSessionRequest
========================
*/
void idSteamNet::OnSessionRequest( SteamNetworkingMessagesSessionRequest_t* param )
{
	const uint64 steamId = param->m_identityRemote.GetSteamID64();
	if( steamId == 0 )
	{
		return;		// not a Steam account
	}

	if( steam_netAcceptAny.GetBool() || Steam_IsLobbyMember( steamId ) )
	{
		AcceptSession( steamId );
		return;
	}

	// Not a member (yet), look again in Update
	STEAM_VERBOSE_PRINT( "STEAM: packet session request from %llu is pending\n", ( unsigned long long )steamId );

	for( int i = 0; i < numPending; i++ )
	{
		if( pending[i].steamId == steamId )
		{
			return;
		}
	}
	if( numPending == MAX_PENDING_REQUESTS )
	{
		// drop the oldest
		for( int i = 1; i < numPending; i++ )
		{
			pending[ i - 1 ] = pending[i];
		}
		numPending--;
	}
	pending[ numPending ].steamId = steamId;
	pending[ numPending ].time = Sys_Milliseconds();
	numPending++;
}

/*
========================
idSteamNet::OnSessionFailed
========================
*/
void idSteamNet::OnSessionFailed( SteamNetworkingMessagesSessionFailed_t* param )
{
	STEAM_VERBOSE_PRINT( "STEAM: packet session with %llu failed: %d %s\n",
						 ( unsigned long long )param->m_info.m_identityRemote.GetSteamID64(), ( int )param->m_info.m_eEndReason, param->m_info.m_szEndDebug );
}

/*
========================
idSteamNet::NoteSendFailure
========================
*/
void idSteamNet::NoteSendFailure( uint64 steamId, EResult result )
{
	// the engine resends, do not flood the console
	if( numFailedSends++ < 8 || steam_verbose.GetBool() )
	{
		idLib::Warning( "STEAM: sending a packet to %llu failed (result %d)", ( unsigned long long )steamId, ( int )result );
	}
}

/*
========================
idSteamNet::Update
========================
*/
void idSteamNet::Update()
{
	const int now = Sys_Milliseconds();

	for( int i = 0; i < numPending; )
	{
		if( Steam_IsLobbyMember( pending[i].steamId ) )
		{
			AcceptSession( pending[i].steamId );
		}
		else if( now - pending[i].time < PENDING_REQUEST_TIMEOUT_MS )
		{
			i++;
			continue;
		}
		pending[i] = pending[ --numPending ];
	}

	if( now - lastCleanup < CLEANUP_INTERVAL_MS )
	{
		return;
	}
	lastCleanup = now;

	for( int i = 0; i < numTracked; )
	{
		if( now - tracked[i].lastUse > IDLE_SESSION_TIMEOUT_MS && !Steam_IsLobbyMember( tracked[i].steamId ) )
		{
			SteamNetworkingIdentity identity;
			identity.SetSteamID64( tracked[i].steamId );
			SteamNetworkingMessages()->CloseSessionWithUser( identity );
			tracked[i] = tracked[ --numTracked ];
			continue;
		}
		i++;
	}
}

/*
========================
idSteamNet::CloseAll
========================
*/
void idSteamNet::CloseAll()
{
	for( int i = 0; i < numTracked; i++ )
	{
		SteamNetworkingIdentity identity;
		identity.SetSteamID64( tracked[i].steamId );
		SteamNetworkingMessages()->CloseSessionWithUser( identity );
	}
	numTracked = 0;
	numPending = 0;
}

/*
========================
Steam_NetReadPacket
========================
*/
bool Steam_NetReadPacket( lobbyAddress_t& from, void* data, int& size, int maxSize )
{
	idSteamNet* net = EnsureNet();
	if( net == NULL )
	{
		return false;
	}

	net->Update();

	ISteamNetworkingMessages* messages = SteamNetworkingMessages();

	for( ;; )
	{
		SteamNetworkingMessage_t* message = NULL;
		if( messages->ReceiveMessagesOnChannel( NET_CHANNEL, &message, 1 ) <= 0 || message == NULL )
		{
			return false;
		}

		const uint64 steamId = message->m_identityPeer.GetSteamID64();
		const int messageSize = message->m_cbSize;

		if( steamId == 0 || messageSize <= 0 || messageSize > maxSize )
		{
			STEAM_VERBOSE_PRINT( "STEAM: dropped a packet of %d bytes from %llu\n", messageSize, ( unsigned long long )steamId );
			message->Release();
			continue;
		}

		memcpy( data, message->m_pData, messageSize );
		size = messageSize;
		from.InitFromStoreFrontId( steamId );
		message->Release();

		net->Touch( steamId );
		return true;
	}
}

/*
========================
Steam_NetSendPacket
========================
*/
bool Steam_NetSendPacket( const lobbyAddress_t& to, const void* data, int size )
{
	if( !to.IsUsingStoreFront() )
	{
		return false;
	}

	idSteamNet* net = EnsureNet();
	if( net == NULL )
	{
		return false;
	}

	if( to.platformId == Steam_GetLocalSteamId() )
	{
		return true;	// ISteamNetworkingMessages can not talk to ourself, the engine never needs it
	}

	SteamNetworkingIdentity identity;
	identity.SetSteamID64( to.platformId );

	const EResult result = SteamNetworkingMessages()->SendMessageToUser( identity, data, ( uint32 )size, NET_SEND_FLAGS, NET_CHANNEL );
	if( result != k_EResultOK )
	{
		net->NoteSendFailure( to.platformId, result );
	}
	else
	{
		net->Touch( to.platformId );
	}
	return true;
}

/*
========================
Steam_NetShutdown
========================
*/
void Steam_NetShutdown()
{
	if( steamNet != NULL )
	{
		steamNet->CloseAll();
		delete steamNet;
		steamNet = NULL;
	}
}