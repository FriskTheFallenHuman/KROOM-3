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

extern unsigned int NetGetVersionChecksum();

idCVar steam_lobbyDistance( "steam_lobbyDistance", "1", CVAR_SYSTEM | CVAR_INTEGER, "lobby search range: 0 = close, 1 = default, 2 = far, 3 = worldwide", 0, 3 );
idCVar steam_lobbyMaxResults( "steam_lobbyMaxResults", "50", CVAR_SYSTEM | CVAR_INTEGER, "maximum number of lobbies a search asks Steam for", 1, 100 );

/*
========================
Lobby data keys
========================
*/
static const char* const LOBBY_KEY_NAME = STEAM_LOBBY_KEY_NAME;		// name of the host
static const char* const LOBBY_KEY_HOST = STEAM_LOBBY_KEY_HOST;		// id of the engine host, used by the search to skip our own lobbies
static const char* const LOBBY_KEY_TYPE = "lobbytype";		// idLobbyBackend::lobbyBackendType_t
static const char* const LOBBY_KEY_NETVER = STEAM_LOBBY_KEY_NETVER;	// NetGetVersionChecksum, a lobby of another build is never returned by the search
static const char* const LOBBY_KEY_SEARCHABLE = "searchable";		// 1 for a lobby FindOrCreateMatch is allowed to return
static const char* const LOBBY_KEY_GAMEMODE = "gamemode";
static const char* const LOBBY_KEY_GAMEMAP = "gamemap";
static const char* const LOBBY_KEY_MAPNAME = "mapname";
static const char* const LOBBY_KEY_FLAGS = "flags";			// idMatchParameters::matchFlags
static const char* const LOBBY_KEY_SKILL = "skill";
static const char* const LOBBY_KEY_INGAME = "ingame";
static const char* const LOBBY_KEY_SESSION = "session";

/*
========================
Registry of the live backends
========================
*/
static const int MAX_STEAM_BACKENDS = 8;
static idLobbyBackendSteam* steamBackends[ MAX_STEAM_BACKENDS ];

static void RegisterBackend( idLobbyBackendSteam* backend )
{
	for( int i = 0; i < MAX_STEAM_BACKENDS; i++ )
	{
		if( steamBackends[i] == NULL )
		{
			steamBackends[i] = backend;
			return;
		}
	}
	assert( !"too many Steam lobby backends" );
}

static void UnregisterBackend( idLobbyBackendSteam* backend )
{
	for( int i = 0; i < MAX_STEAM_BACKENDS; i++ )
	{
		if( steamBackends[i] == backend )
		{
			steamBackends[i] = NULL;
		}
	}
}

/*
========================
Steam_IsLobbyMember
========================
*/
bool Steam_IsLobbyMember( uint64 steamId )
{
	const CSteamID id( steamId );
	for( int i = 0; i < MAX_STEAM_BACKENDS; i++ )
	{
		if( steamBackends[i] != NULL && steamBackends[i]->HasMember( id ) )
		{
			return true;
		}
	}
	return false;
}

/*
========================
SetLobbyKey
========================
*/
static void SetLobbyKey( const CSteamID& lobby, const char* key, const char* value )
{
	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking == NULL )
	{
		return;
	}

	const char* current = matchmaking->GetLobbyData( lobby, key );
	if( current != NULL && idStr::Cmp( current, value ) == 0 )
	{
		return;
	}

	if( !matchmaking->SetLobbyData( lobby, key, value ) )
	{
		idLib::Warning( "STEAM: SetLobbyData( %s ) failed", key );
	}
}

/*
========================
idLobbyBackendSteam::idLobbyBackendSteam
========================
*/
idLobbyBackendSteam::idLobbyBackendSteam() :
	state( STATE_INVALID ),
	ownerAddressOut( NULL ),
	skill( 0.0f ),
	joinable( true ),
	inGame( false ),
	sessionStarted( false ),
	entered( false ),
	browse(),
	browseApplied( false )
{
	RegisterBackend( this );
}

/*
========================
idLobbyBackendSteam::~idLobbyBackendSteam
========================
*/
idLobbyBackendSteam::~idLobbyBackendSteam()
{
	UnregisterBackend( this );
}

/*
========================
idLobbyBackendSteam::GetSteamLobbyType
========================
*/
ELobbyType idLobbyBackendSteam::GetSteamLobbyType() const
{
	if( MatchTypeInviteOnly( parms.matchFlags ) && type == TYPE_PARTY )
	{
		return k_ELobbyTypePrivate;
	}
	if( type == TYPE_PARTY )
	{
		return k_ELobbyTypeFriendsOnly;
	}
	return k_ELobbyTypePublic;
}

/*
========================
idLobbyBackendSteam::CreateSteamLobby
========================
*/
void idLobbyBackendSteam::CreateSteamLobby()
{
	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking == NULL )
	{
		state = STATE_FAILED;
		return;
	}

	// Steam counts accounts, split screen users share one, so this is an upper bound of what the engine allows
	const int maxMembers = Max< int >( ( int )parms.numSlots, 1 );

	const SteamAPICall_t call = matchmaking->CreateLobby( GetSteamLobbyType(), maxMembers );
	if( call == k_uAPICallInvalid )
	{
		state = STATE_FAILED;
		return;
	}

	state = STATE_CREATING;
	createResult.Set( call, this, &idLobbyBackendSteam::OnLobbyCreated );
}

/*
========================
idLobbyBackendSteam::OnLobbyCreated
========================
*/
void idLobbyBackendSteam::OnLobbyCreated( LobbyCreated_t* param, bool ioFailure )
{
	if( ioFailure || param->m_eResult != k_EResultOK )
	{
		idLib::Warning( "STEAM: CreateLobby failed (%s, result %d)", ioFailure ? "io failure" : "reply", ioFailure ? 0 : ( int )param->m_eResult );
		state = STATE_FAILED;
		return;
	}

	lobbyId = CSteamID( param->m_ulSteamIDLobby );
	entered = true;
	browseApplied = false;		// a new lobby starts with the type of GetSteamLobbyType

	STEAM_VERBOSE_PRINT( "STEAM: created %s lobby %llu\n", GetLobbyTypeString(), ( unsigned long long )lobbyId.ConvertToUint64() );

	PublishLobbyData();

	// The lobby of a migration is ready, the old one is of no use any more
	if( previousLobbyId.IsValid() )
	{
		SteamMatchmaking()->LeaveLobby( previousLobbyId );
		previousLobbyId = CSteamID();
	}

	state = STATE_READY;
}

/*
========================
idLobbyBackendSteam::PublishLobbyData
========================
*/
void idLobbyBackendSteam::PublishLobbyData()
{
	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking == NULL || !isHost || !lobbyId.IsValid() )
	{
		return;
	}

	SetLobbyKey( lobbyId, LOBBY_KEY_NAME, Steam_GetPersonaName() );
	SetLobbyKey( lobbyId, LOBBY_KEY_HOST, va( "%llu", ( unsigned long long )Steam_GetLocalSteamId() ) );
	SetLobbyKey( lobbyId, LOBBY_KEY_TYPE, va( "%d", ( int )type ) );
	SetLobbyKey( lobbyId, LOBBY_KEY_NETVER, va( "%u", NetGetVersionChecksum() ) );
	SetLobbyKey( lobbyId, LOBBY_KEY_SEARCHABLE, ( type == TYPE_GAME && MatchTypeIsSearchable( parms.matchFlags ) && !MatchTypeInviteOnly( parms.matchFlags ) ) ? "1" : "0" );
	SetLobbyKey( lobbyId, LOBBY_KEY_GAMEMODE, va( "%d", ( int )parms.gameMode ) );
	SetLobbyKey( lobbyId, LOBBY_KEY_GAMEMAP, va( "%d", ( int )parms.gameMap ) );
	SetLobbyKey( lobbyId, LOBBY_KEY_MAPNAME, parms.mapName.c_str() );
	SetLobbyKey( lobbyId, LOBBY_KEY_FLAGS, va( "%d", ( int )parms.matchFlags ) );
	SetLobbyKey( lobbyId, LOBBY_KEY_SKILL, va( "%d", ( int )skill ) );
	SetLobbyKey( lobbyId, LOBBY_KEY_INGAME, inGame ? "1" : "0" );
	SetLobbyKey( lobbyId, LOBBY_KEY_SESSION, sessionStarted ? "1" : "0" );

	ApplyBrowseListing();

	matchmaking->SetLobbyJoinable( lobbyId, joinable );
}

/*
========================
idLobbyBackendSteam::ApplyBrowseListing
========================
*/
void idLobbyBackendSteam::ApplyBrowseListing()
{
	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking == NULL || !isHost || !lobbyId.IsValid() || type != TYPE_PARTY )
	{
		return;
	}

	const bool listed = browse.listed;
	if( listed != browseApplied )
	{
		matchmaking->SetLobbyType( lobbyId, listed ? k_ELobbyTypePublic : GetSteamLobbyType() );
		browseApplied = listed;
	}

	SetLobbyKey( lobbyId, STEAM_LOBBY_KEY_BROWSE, listed ? "1" : "0" );
	if( listed )
	{
		SetLobbyKey( lobbyId, STEAM_LOBBY_KEY_BROWSE_MODE, va( "%d", browse.gameMode ) );
		SetLobbyKey( lobbyId, STEAM_LOBBY_KEY_BROWSE_MAP, va( "%d", browse.gameMap ) );
		SetLobbyKey( lobbyId, STEAM_LOBBY_KEY_BROWSE_PLAYERS, va( "%d", browse.numPlayers ) );
		SetLobbyKey( lobbyId, STEAM_LOBBY_KEY_BROWSE_MAX, va( "%d", browse.maxPlayers ) );
		SetLobbyKey( lobbyId, STEAM_LOBBY_KEY_BROWSE_JOINABLE, browse.joinable ? "1" : "0" );
	}
}

/*
========================
idLobbyBackendSteam::SetBrowseListing
========================
*/
void idLobbyBackendSteam::SetBrowseListing( const steamBrowseListing_t& listing )
{
	if( type != TYPE_PARTY )
	{
		return;
	}

	if( listing.listed == browse.listed && listing.gameMode == browse.gameMode && listing.gameMap == browse.gameMap &&
			listing.numPlayers == browse.numPlayers && listing.maxPlayers == browse.maxPlayers && listing.joinable == browse.joinable )
	{
		return;
	}

	browse = listing;
	ApplyBrowseListing();
}

/*
========================
Steam_SetBrowseListing
========================
*/
void Steam_SetBrowseListing( const steamBrowseListing_t& listing )
{
	for( int i = 0; i < MAX_STEAM_BACKENDS; i++ )
	{
		if( steamBackends[i] != NULL )
		{
			steamBackends[i]->SetBrowseListing( listing );
		}
	}
}

/*
========================
idLobbyBackendSteam::StartHosting
========================
*/
void idLobbyBackendSteam::StartHosting( const idMatchParameters& p, float skillLevel, lobbyBackendType_t lobbyType )
{
	NET_VERBOSE_PRINT( "idLobbyBackendSteam::StartHosting\n" );

	parms = p;
	skill = skillLevel;
	isLocal = false;
	isHost = true;

	CreateSteamLobby();
}

/*
========================
idLobbyBackendSteam::StartFinding
========================
*/
void idLobbyBackendSteam::StartFinding( const idMatchParameters& p, int numPartyUsers, float skillLevel )
{
	parms = p;
	skill = skillLevel;
	isLocal	= false;
	isHost = false;

	searchResults.Clear();

	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking == NULL )
	{
		state = STATE_FAILED;
		return;
	}

	matchmaking->AddRequestLobbyListStringFilter( LOBBY_KEY_NETVER, va( "%u", NetGetVersionChecksum() ), k_ELobbyComparisonEqual );
	matchmaking->AddRequestLobbyListNumericalFilter( LOBBY_KEY_TYPE, ( int )TYPE_GAME, k_ELobbyComparisonEqual );
	matchmaking->AddRequestLobbyListNumericalFilter( LOBBY_KEY_SEARCHABLE, 1, k_ELobbyComparisonEqual );
	if( p.gameMode != GAME_MODE_RANDOM )
	{
		matchmaking->AddRequestLobbyListNumericalFilter( LOBBY_KEY_GAMEMODE, ( int )p.gameMode, k_ELobbyComparisonEqual );
	}
	if( p.gameMap != GAME_MAP_RANDOM )
	{
		matchmaking->AddRequestLobbyListNumericalFilter( LOBBY_KEY_GAMEMAP, ( int )p.gameMap, k_ELobbyComparisonEqual );
	}
	matchmaking->AddRequestLobbyListFilterSlotsAvailable( Max( numPartyUsers, 1 ) );
	matchmaking->AddRequestLobbyListNearValueFilter( LOBBY_KEY_SKILL, ( int )skillLevel );
	matchmaking->AddRequestLobbyListDistanceFilter( ( ELobbyDistanceFilter )idMath::ClampInt( k_ELobbyDistanceFilterClose, k_ELobbyDistanceFilterWorldwide, steam_lobbyDistance.GetInteger() ) );
	matchmaking->AddRequestLobbyListResultCountFilter( steam_lobbyMaxResults.GetInteger() );

	const SteamAPICall_t call = matchmaking->RequestLobbyList();
	if( call == k_uAPICallInvalid )
	{
		state = STATE_FAILED;
		return;
	}

	state = STATE_SEARCHING;
	listResult.Set( call, this, &idLobbyBackendSteam::OnLobbyListReceived );
}

/*
========================
idLobbyBackendSteam::OnLobbyListReceived
========================
*/
void idLobbyBackendSteam::OnLobbyListReceived( LobbyMatchList_t* param, bool ioFailure )
{
	if( ioFailure )
	{
		idLib::Warning( "STEAM: RequestLobbyList failed" );
		state = STATE_FAILED;
		return;
	}

	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	const uint64 localId = Steam_GetLocalSteamId();

	searchResults.Clear();
	for( uint32 i = 0; i < param->m_nLobbiesMatching; i++ )
	{
		const CSteamID candidate = matchmaking->GetLobbyByIndex( ( int )i );
		if( !candidate.IsValid() )
		{
			continue;
		}

		// our own lobbies, a search started by the party host while it hosts a game lobby would find it
		const char* host = matchmaking->GetLobbyData( candidate, LOBBY_KEY_HOST );
		if( host != NULL && host[0] != '\0' && strtoull( host, NULL, 10 ) == localId )
		{
			continue;
		}

		searchResults.Append( candidate );
	}

	STEAM_VERBOSE_PRINT( "STEAM: lobby search returned %d of %u lobbies\n", searchResults.Num(), param->m_nLobbiesMatching );

	// an empty list is not a failure, idLobby treats it as one
	state = STATE_READY;
}

/*
========================
idLobbyBackendSteam::GetSearchResults
========================
*/
void idLobbyBackendSteam::GetSearchResults( idList< lobbyConnectInfo_t >& results )
{
	results.Clear();
	for( int i = 0; i < searchResults.Num(); i++ )
	{
		lobbyConnectInfo_t info;
		info.platformLobbyId = searchResults[i].ConvertToUint64();
		results.Append( info );
	}
}

/*
========================
idLobbyBackendSteam::JoinFromConnectInfo
========================
*/
void idLobbyBackendSteam::JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo )
{
	isLocal	= false;
	isHost	= false;

	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking == NULL || connectInfo.platformLobbyId == 0 )
	{
		state = STATE_FAILED;
		return;
	}

	lobbyId = CSteamID( connectInfo.platformLobbyId );
	if( !lobbyId.IsLobby() )
	{
		idLib::Warning( "STEAM: %llu is not a lobby id", ( unsigned long long )connectInfo.platformLobbyId );
		lobbyId = CSteamID();
		state = STATE_FAILED;
		return;
	}

	const SteamAPICall_t call = matchmaking->JoinLobby( lobbyId );
	if( call == k_uAPICallInvalid )
	{
		lobbyId = CSteamID();
		state = STATE_FAILED;
		return;
	}

	STEAM_VERBOSE_PRINT( "STEAM: joining lobby %llu\n", ( unsigned long long )lobbyId.ConvertToUint64() );

	state = STATE_OBTAINING_ADDRESS;
	enterResult.Set( call, this, &idLobbyBackendSteam::OnLobbyEntered );
}

/*
========================
idLobbyBackendSteam::OnLobbyEntered
========================
*/
void idLobbyBackendSteam::OnLobbyEntered( LobbyEnter_t* param, bool ioFailure )
{
	if( ioFailure || param->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess )
	{
		idLib::Warning( "STEAM: JoinLobby failed (%s, response %u)", ioFailure ? "io failure" : "reply", ioFailure ? 0u : ( unsigned )param->m_EChatRoomEnterResponse );
		lobbyId = CSteamID();
		state = STATE_FAILED;
		return;
	}

	entered = true;

	if( ownerAddressOut != NULL )
	{
		ResolveOwnerAddress();
	}
}

/*
========================
idLobbyBackendSteam::GetOwnerAddress
========================
*/
void idLobbyBackendSteam::GetOwnerAddress( lobbyAddress_t& outAddr )
{
	ownerAddressOut = &outAddr;

	if( entered )
	{
		ResolveOwnerAddress();
	}
}

/*
========================
idLobbyBackendSteam::ResolveOwnerAddress
========================
*/
void idLobbyBackendSteam::ResolveOwnerAddress()
{
	assert( entered && ownerAddressOut != NULL );

	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	const CSteamID owner = ( matchmaking != NULL ) ? matchmaking->GetLobbyOwner( lobbyId ) : CSteamID();

	if( !owner.IsValid() || owner.ConvertToUint64() == Steam_GetLocalSteamId() )
	{
		idLib::Warning( "STEAM: lobby %llu has no usable owner", ( unsigned long long )lobbyId.ConvertToUint64() );
		state = STATE_FAILED;
		return;
	}

	ownerAddressOut->InitFromStoreFrontId( owner.ConvertToUint64() );
	state = STATE_READY;
}

/*
========================
idLobbyBackendSteam::GetConnectInfo
========================
*/
lobbyConnectInfo_t idLobbyBackendSteam::GetConnectInfo()
{
	lobbyConnectInfo_t connectInfo;
	connectInfo.platformLobbyId = lobbyId.ConvertToUint64();
	return connectInfo;
}

/*
========================
idLobbyBackendSteam::IsOwnerOfConnectInfo
========================
*/
bool idLobbyBackendSteam::IsOwnerOfConnectInfo( const lobbyConnectInfo_t& connectInfo ) const
{
	return lobbyId.IsValid() && connectInfo.platformLobbyId == lobbyId.ConvertToUint64();
}

/*
========================
idLobbyBackendSteam::HasMember
========================
*/
bool idLobbyBackendSteam::HasMember( const CSteamID& id ) const
{
	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking == NULL )
	{
		return false;
	}

	const CSteamID lobbies[2] = { lobbyId, previousLobbyId };
	for( int l = 0; l < 2; l++ )
	{
		if( !lobbies[l].IsValid() )
		{
			continue;
		}
		const int numMembers = matchmaking->GetNumLobbyMembers( lobbies[l] );
		for( int m = 0; m < numMembers; m++ )
		{
			if( matchmaking->GetLobbyMemberByIndex( lobbies[l], m ) == id )
			{
				return true;
			}
		}
	}
	return false;
}

/*
========================
idLobbyBackendSteam::IsLobbyOwner
========================
*/
bool idLobbyBackendSteam::IsLobbyOwner() const
{
	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	return matchmaking != NULL && lobbyId.IsValid() && matchmaking->GetLobbyOwner( lobbyId ).ConvertToUint64() == Steam_GetLocalSteamId();
}

/*
========================
idLobbyBackendSteam::LeaveSteamLobby
========================
*/
void idLobbyBackendSteam::LeaveSteamLobby()
{
	ISteamMatchmaking* matchmaking = SteamMatchmaking();
	if( matchmaking != NULL )
	{
		if( lobbyId.IsValid() )
		{
			matchmaking->LeaveLobby( lobbyId );
		}
		if( previousLobbyId.IsValid() )
		{
			matchmaking->LeaveLobby( previousLobbyId );
		}
	}

	lobbyId = CSteamID();
	previousLobbyId = CSteamID();
	entered = false;
}

/*
========================
idLobbyBackendSteam::Shutdown
========================
*/
void idLobbyBackendSteam::Shutdown()
{
	createResult.Cancel();
	enterResult.Cancel();
	listResult.Cancel();

	LeaveSteamLobby();

	ownerAddressOut = NULL;
	searchResults.Clear();
	sessionStarted = false;
	state = STATE_SHUTDOWN;
}

/*
========================
idLobbyBackendSteam::SetIsJoinable
========================
*/
void idLobbyBackendSteam::SetIsJoinable( bool value )
{
	joinable = value;

	if( isHost && lobbyId.IsValid() && SteamMatchmaking() != NULL )
	{
		SteamMatchmaking()->SetLobbyJoinable( lobbyId, joinable );
	}
}

/*
========================
idLobbyBackendSteam::Pump
========================
*/
void idLobbyBackendSteam::Pump()
{
}

/*
========================
idLobbyBackendSteam::UpdateMatchParms
========================
*/
void idLobbyBackendSteam::UpdateMatchParms( const idMatchParameters& p )
{
	parms = p;
	PublishLobbyData();
}

/*
========================
idLobbyBackendSteam::UpdateLobbySkill
========================
*/
void idLobbyBackendSteam::UpdateLobbySkill( float lobbySkill )
{
	skill = lobbySkill;

	if( isHost && lobbyId.IsValid() )
	{
		SetLobbyKey( lobbyId, LOBBY_KEY_SKILL, va( "%d", ( int )skill ) );
	}
}

/*
========================
idLobbyBackendSteam::SetInGame
========================
*/
void idLobbyBackendSteam::SetInGame( bool value )
{
	inGame = value;

	if( isHost && lobbyId.IsValid() )
	{
		SetLobbyKey( lobbyId, LOBBY_KEY_INGAME, inGame ? "1" : "0" );
	}
}

/*
========================
idLobbyBackendSteam::StartSession
========================
*/
void idLobbyBackendSteam::StartSession()
{
	sessionStarted = true;

	if( isHost && lobbyId.IsValid() )
	{
		SetLobbyKey( lobbyId, LOBBY_KEY_SESSION, "1" );
	}
}

/*
========================
idLobbyBackendSteam::EndSession
========================
*/
void idLobbyBackendSteam::EndSession()
{
	sessionStarted = false;

	if( isHost && lobbyId.IsValid() )
	{
		SetLobbyKey( lobbyId, LOBBY_KEY_SESSION, "0" );
	}
}

/*
========================
idLobbyBackendSteam::FlushStats
========================
*/
void idLobbyBackendSteam::FlushStats()
{
	Steam_StoreStats();
}

/*
========================
idLobbyBackendSteam::BecomeHost
========================
*/
void idLobbyBackendSteam::BecomeHost( int numInvites )
{
	isHost = true;

	createResult.Cancel();

	// the old lobby is left when the new one exists, until then its members are still known peers
	if( lobbyId.IsValid() )
	{
		if( previousLobbyId.IsValid() && SteamMatchmaking() != NULL )
		{
			SteamMatchmaking()->LeaveLobby( previousLobbyId );
		}
		previousLobbyId = lobbyId;
	}
	lobbyId = CSteamID();
	entered = false;
	ownerAddressOut = NULL;

	CreateSteamLobby();
}
