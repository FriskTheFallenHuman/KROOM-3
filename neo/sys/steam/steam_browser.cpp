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
#include "steam_lobby_backend.h"

extern idCVar steam_lobbyDistance;
extern idCVar steam_lobbyMaxResults;
extern unsigned int NetGetVersionChecksum();

/*
================================================================================================

	Game browser

================================================================================================
*/

/*
================================================
idSteamBrowser
================================================
*/
class idSteamBrowser
{
public:
	idSteamBrowser() : callback( NULL ) {}
	~idSteamBrowser()
	{
		Cancel();
	}

	void				Refresh( const idCallback& cb );
	void				Cancel();

	int					NumServers() const
	{
		return entries.Num();
	}
	const serverInfo_t*	ServerInfo( int i ) const
	{
		return ( i >= 0 && i < entries.Num() ) ? &entries[i].info : NULL;
	}
	uint64				LobbyId( int i ) const
	{
		return ( i >= 0 && i < entries.Num() ) ? entries[i].lobbyId : 0;
	}
	uint64				HostId( int i ) const
	{
		return ( i >= 0 && i < entries.Num() ) ? entries[i].hostId : 0;
	}

private:
	struct entry_t
	{
		serverInfo_t	info;
		uint64			lobbyId;
		uint64			hostId;
	};

	void				OnLobbyList( LobbyMatchList_t* param, bool ioFailure );
	void				Deliver();

	CCallResult< idSteamBrowser, LobbyMatchList_t >	listResult;
	idList< entry_t >	entries;
	idCallback*			callback;		// owned, called once per request
};

static idSteamBrowser* steamBrowser = NULL;

/*
========================
EnsureBrowser
========================
*/
static idSteamBrowser* EnsureBrowser()
{
	if( steamBrowser == NULL )
	{
		if( !Steam_IsRunning() || SteamMatchmaking() == NULL )
		{
			return NULL;
		}
		steamBrowser = new( TAG_NETWORKING ) idSteamBrowser();
	}
	return steamBrowser;
}

/*
========================
idSteamBrowser::Refresh
========================
*/
void idSteamBrowser::Refresh( const idCallback& cb )
{
	Cancel();
	entries.Clear();
	callback = cb.Clone();

	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking == NULL )
	{
		Deliver();
		return;
	}

	// only lobbies of this build that a host listed, with room for one more player.
	matchmaking->AddRequestLobbyListStringFilter( STEAM_LOBBY_KEY_NETVER, va( "%u", NetGetVersionChecksum() ), k_ELobbyComparisonEqual );
	matchmaking->AddRequestLobbyListNumericalFilter( STEAM_LOBBY_KEY_BROWSE, 1, k_ELobbyComparisonEqual );
	matchmaking->AddRequestLobbyListFilterSlotsAvailable( 1 );
	matchmaking->AddRequestLobbyListDistanceFilter( ( ELobbyDistanceFilter )idMath::ClampInt( k_ELobbyDistanceFilterClose, k_ELobbyDistanceFilterWorldwide, steam_lobbyDistance.GetInteger() ) );
	matchmaking->AddRequestLobbyListResultCountFilter( steam_lobbyMaxResults.GetInteger() );

	const SteamAPICall_t call = matchmaking->RequestLobbyList();
	if( call == k_uAPICallInvalid )
	{
		idLib::Warning( "STEAM: could not request the lobby list for the game browser" );
		Deliver();
		return;
	}

	listResult.Set( call, this, &idSteamBrowser::OnLobbyList );
}

/*
========================
idSteamBrowser::Cancel
========================
*/
void idSteamBrowser::Cancel()
{
	listResult.Cancel();

	if( callback != NULL )
	{
		delete callback;
		callback = NULL;
	}
}

/*
========================
idSteamBrowser::Deliver
========================
*/
void idSteamBrowser::Deliver()
{
	idCallback* cb = callback;
	callback = NULL;

	if( cb != NULL )
	{
		cb->Call();
		delete cb;
	}
}

/*
========================
idSteamBrowser::OnLobbyList
========================
*/
void idSteamBrowser::OnLobbyList( LobbyMatchList_t* param, bool ioFailure )
{
	entries.Clear();

	if( ioFailure )
	{
		idLib::Warning( "STEAM: RequestLobbyList failed for the game browser" );
		Deliver();
		return;
	}

	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	const uint64 localId = Steam_GetLocalSteamId();

	for( uint32 i = 0; i < param->m_nLobbiesMatching; i++ )
	{
		const CSteamID lobby = matchmaking->GetLobbyByIndex( ( int )i );
		if( !lobby.IsValid() )
		{
			continue;
		}

		const char* host = matchmaking->GetLobbyData( lobby, STEAM_LOBBY_KEY_HOST );
		const uint64 hostId = ( host != NULL && host[0] != '\0' ) ? strtoull( host, NULL, 10 ) : 0;
		if( hostId != 0 && hostId == localId )
		{
			continue;		// our own game
		}

		entry_t& entry = entries.Alloc();
		entry.lobbyId	= lobby.ConvertToUint64();
		entry.hostId	= hostId;

		// Lobby data is written by the lobby owner only.
		const char* name = matchmaking->GetLobbyData( lobby, STEAM_LOBBY_KEY_NAME );
		entry.info.serverName	= ( name != NULL ) ? name : "";
		if( entry.info.serverName.IsEmpty() )
		{
			entry.info.serverName = va( "%llu", ( unsigned long long )entry.hostId );
		}
		entry.info.gameMode = ( int8 )idMath::ClampInt( -1, 127, atoi( matchmaking->GetLobbyData( lobby, STEAM_LOBBY_KEY_BROWSE_MODE ) ) );
		entry.info.gameMap = ( int8 )idMath::ClampInt( -1, 127, atoi( matchmaking->GetLobbyData( lobby, STEAM_LOBBY_KEY_BROWSE_MAP ) ) );
		entry.info.numPlayers = idMath::ClampInt( 0, 64, atoi( matchmaking->GetLobbyData( lobby, STEAM_LOBBY_KEY_BROWSE_PLAYERS ) ) );
		entry.info.maxPlayers = idMath::ClampInt( 0, 64, atoi( matchmaking->GetLobbyData( lobby, STEAM_LOBBY_KEY_BROWSE_MAX ) ) );
		entry.info.joinable = atoi( matchmaking->GetLobbyData( lobby, STEAM_LOBBY_KEY_BROWSE_JOINABLE ) ) != 0;
	}

	STEAM_VERBOSE_PRINT( "STEAM: game browser: %d of %u lobbies\n", entries.Num(), param->m_nLobbiesMatching );

	Deliver();
}

/*
========================
Steam_BrowserRefresh
========================
*/
void Steam_BrowserRefresh( const idCallback& callback )
{
	idSteamBrowser* browser = EnsureBrowser();
	if( browser == NULL )
	{
		idCallback* cb = callback.Clone();
		if( cb != NULL )
		{
			cb->Call();
			delete cb;
		}
		return;
	}
	browser->Refresh( callback );
}

/*
========================
Steam_BrowserCancel
========================
*/
void Steam_BrowserCancel()
{
	if( steamBrowser != NULL )
	{
		steamBrowser->Cancel();
	}
}

/*
========================
Steam_BrowserNumServers
========================
*/
int Steam_BrowserNumServers()
{
	return ( steamBrowser != NULL ) ? steamBrowser->NumServers() : 0;
}

/*
========================
Steam_BrowserServerInfo
========================
*/
const serverInfo_t* Steam_BrowserServerInfo( int i )
{
	return ( steamBrowser != NULL ) ? steamBrowser->ServerInfo( i ) : NULL;
}

/*
========================
Steam_BrowserLobbyId
========================
*/
uint64 Steam_BrowserLobbyId( int i )
{
	return ( steamBrowser != NULL ) ? steamBrowser->LobbyId( i ) : 0;
}

/*
========================
Steam_BrowserHostId
========================
*/
uint64 Steam_BrowserHostId( int i )
{
	return ( steamBrowser != NULL ) ? steamBrowser->HostId( i ) : 0;
}

/*
========================
Steam_BrowserShutdown
========================
*/
void Steam_BrowserShutdown()
{
	delete steamBrowser;
	steamBrowser = NULL;
}