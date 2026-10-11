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

#include "steam_local.h"

idCVar steam_enable( "steam_enable", "1", CVAR_SYSTEM | CVAR_BOOL | CVAR_INIT | CVAR_NOCHEAT, "enable steam, set to 0 at startup to use the offline mode" );
idCVar steam_verbose( "steam_verbose", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_NOCHEAT, "print steam networking and lobby debug messages" );
idCVar steam_lobbies( "steam_lobbies", "1", CVAR_SYSTEM | CVAR_BOOL | CVAR_NOCHEAT, "use Steam lobbies and packet transport for online matches; 0 uses direct UDP" );
idCVar steam_relayOnly( "steam_relayOnly", "1", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE | CVAR_NOCHEAT, "hide player IP addresses using Steam Relay; may increase latency. Applies to new sessions" );
idCVar steam_dlcMaps( "steam_dlcMaps", "", CVAR_SYSTEM | CVAR_NOCHEAT, "space-separated map prefix:DLC app ID pairs, e.g. \"maps/game/erebus:123456 maps/game/le_:123457\"" );

static bool steamRunning = false;

/*
================================================
idSteamCallbacks
================================================
*/
class idSteamCallbacks
{
public:
	idSteamCallbacks() : lobbyJoinRequest( 0 ), overlayActive( false ), overlayChanged( false ) {}

	uint64	PopLobbyJoinRequest()
	{
		uint64 id = lobbyJoinRequest;
		lobbyJoinRequest = 0;
		return id;
	}
	void	SetLobbyJoinRequest( uint64 id )
	{
		lobbyJoinRequest = id;
	}

	// The overlay pauses the game the same way a console does
	void	UpdateOverlay()
	{
		if( overlayChanged && session != NULL )
		{
			overlayChanged = false;
			session->SetSystemUIShowing( overlayActive );
		}
	}

private:
	STEAM_CALLBACK( idSteamCallbacks, OnGameOverlayActivated, GameOverlayActivated_t );
	STEAM_CALLBACK( idSteamCallbacks, OnGameLobbyJoinRequested, GameLobbyJoinRequested_t );

	uint64	lobbyJoinRequest;
	bool	overlayActive;
	bool	overlayChanged;
};

static idSteamCallbacks* steamCallbacks = NULL;

/*
========================
idSteamCallbacks::OnGameOverlayActivated
========================
*/
void idSteamCallbacks::OnGameOverlayActivated( GameOverlayActivated_t* param )
{
	overlayActive = ( param->m_bActive != 0 );
	overlayChanged = true;
}

/*
========================
idSteamCallbacks::OnGameLobbyJoinRequested

The player accepted an invite or used "Join game" in the friends list while the game is running.
========================
*/
void idSteamCallbacks::OnGameLobbyJoinRequested( GameLobbyJoinRequested_t* param )
{
	STEAM_VERBOSE_PRINT( "STEAM: join requested for lobby %llu by friend %llu\n", ( unsigned long long )param->m_steamIDLobby.ConvertToUint64(), ( unsigned long long )param->m_steamIDFriend.ConvertToUint64() );
	lobbyJoinRequest = param->m_steamIDLobby.ConvertToUint64();
}

/*
========================
ApplyNetworkConfig
========================
*/
static void ApplyNetworkConfig()
{
	steam_relayOnly.ClearModified();

	ISteamNetworkingUtils* utils = SteamNetworkingUtils();
	if( utils == NULL )
	{
		return;
	}

	utils->InitRelayNetworkAccess();

	const int32 iceMode = steam_relayOnly.GetBool() ? k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_Disable : k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_Default;
	if( !utils->SetGlobalConfigValueInt32( k_ESteamNetworkingConfig_P2P_Transport_ICE_Enable, iceMode ) )
	{
		idLib::Warning( "STEAM: could not set the P2P transport mode, peer IP addresses may be shared" );
	}
}

/*
========================
Steam_Init
========================
*/
steamInitResult_t Steam_Init()
{
	if( steamRunning )
	{
		return STEAM_INIT_OK;
	}

	if( !steam_enable.GetBool() )
	{
		idLib::Printf( "Steamworks disabled (steam_enable 0)\n" );
		return STEAM_INIT_DISABLED;
	}

#if STEAM_APP_ID != 208200
	if( SteamAPI_RestartAppIfNecessary( STEAM_APP_ID ) )
	{
		return STEAM_INIT_RESTART;
	}
#endif

	SteamErrMsg errMsg = { 0 };
	const ESteamAPIInitResult result = SteamAPI_InitEx( &errMsg );
	if( result != k_ESteamAPIInitResult_OK )
	{
		idLib::Warning( "Steam init failed (%d): %s", ( int )result, errMsg );
		return STEAM_INIT_FAILED;
	}

	steamRunning = true;
	steamCallbacks = new( TAG_SYSTEM ) idSteamCallbacks();

	ApplyNetworkConfig();

	idLib::Printf( "Steam initialized, logged in as '%s' (%llu), App ID %u\n", Steam_GetPersonaName(), ( unsigned long long )Steam_GetLocalSteamId(), ( unsigned int )SteamUtils()->GetAppID() );

	return STEAM_INIT_OK;
}

/*
========================
Steam_IsRunning
========================
*/
bool Steam_IsRunning()
{
	return steamRunning;
}

/*
========================
Steam_UseLobbies
Not while Steam is in offline mode: a lobby can not be created then, the direct backend keeps working.
========================
*/
bool Steam_UseLobbies()
{
	return steamRunning && steam_lobbies.GetBool() && SteamUser() != NULL && SteamUser()->BLoggedOn();
}

/*
========================
Steam_RunCallbacks
========================
*/
void Steam_RunCallbacks()
{
	if( !steamRunning )
	{
		return;
	}

	SteamAPI_RunCallbacks();

	if( steam_relayOnly.IsModified() )
	{
		ApplyNetworkConfig();
	}

	if( steamCallbacks != NULL )
	{
		steamCallbacks->UpdateOverlay();
	}

	// the finished leaderboard requests are handed to their callers and deleted here
	Steam_LeaderboardPump();
}

/*
========================
Steam_Shutdown
========================
*/
void Steam_Shutdown()
{
	if( !steamRunning )
	{
		return;
	}

	Steam_LeaderboardShutdown();
	Steam_BrowserShutdown();
	Steam_NetShutdown();

	delete steamCallbacks;
	steamCallbacks = NULL;

	SteamAPI_Shutdown();
	steamRunning = false;
}

/*
========================
Steam_GetLocalSteamId
========================
*/
uint64 Steam_GetLocalSteamId()
{
	if( !steamRunning || SteamUser() == NULL )
	{
		return 0;
	}
	return SteamUser()->GetSteamID().ConvertToUint64();
}

/*
========================
Steam_GetPersonaName
========================
*/
const char* Steam_GetPersonaName()
{
	if( !steamRunning || SteamFriends() == NULL )
	{
		return "";
	}
	return SteamFriends()->GetPersonaName();
}

/*
========================
Steam_PopLobbyJoinRequest
========================
*/
uint64 Steam_PopLobbyJoinRequest()
{
	return ( steamCallbacks != NULL ) ? steamCallbacks->PopLobbyJoinRequest() : 0;
}

/*
========================
Steam_HashSteamId
========================
*/
int Steam_HashSteamId( uint64 steamId )
{
	return ( int )( ( uint32 )steamId ^ ( uint32 )( steamId >> 32 ) );
}

/*
========================
Steam_UpdatePresence
========================
*/
void Steam_UpdatePresence( const steamPresence_t& presence )
{
	if( !steamRunning || SteamFriends() == NULL )
	{
		return;
	}

	static steamPresence_t last;
	static bool haveLast = false;

	if( haveLast && last.state == presence.state && last.partyLobbyId == presence.partyLobbyId && last.numPlayers == presence.numPlayers && last.maxPlayers == presence.maxPlayers )
	{
		return;
	}
	last = presence;
	haveLast = true;

	const char* token = "#Status_Menu";
	switch( presence.state )
	{
		case idSession::PARTY_LOBBY:
			token = "#Status_PartyLobby";
			break;
		case idSession::GAME_LOBBY:
			token = "#Status_GameLobby";
			break;
		case idSession::LOADING:
			token = "#Status_Loading";
			break;
		case idSession::INGAME:
		case idSession::BUSY:
			token = "#Status_InGame";
			break;
		case idSession::SEARCHING:
		case idSession::CONNECTING:
			token = "#Status_Connecting";
			break;
		case idSession::IDLE:
		default:
			token = "#Status_Menu";
			break;
	}

	ISteamFriends* friends = SteamFriends();
	friends->SetRichPresence( "steam_display", token );

	if( presence.partyLobbyId != 0 )
	{
		// "Join Game" and "Invite to game" in the friends list
		friends->SetRichPresence( "connect", va( "+connect_lobby %llu", ( unsigned long long )presence.partyLobbyId ) );
		friends->SetRichPresence( "steam_player_group", va( "%llu", ( unsigned long long )presence.partyLobbyId ) );
		friends->SetRichPresence( "steam_player_group_size", va( "%d", presence.numPlayers ) );
	}
	else
	{
		friends->ClearRichPresence();
		friends->SetRichPresence( "steam_display", token );
	}

	STEAM_VERBOSE_PRINT( "STEAM: rich presence %s, party lobby %llu, %d/%d players\n", token, ( unsigned long long )presence.partyLobbyId, presence.numPlayers, presence.maxPlayers );
}

/*
========================
Steam_ShowInviteDialog
========================
*/
void Steam_ShowInviteDialog( uint64 lobbyId )
{
	if( steamRunning && lobbyId != 0 )
	{
		SteamFriends()->ActivateGameOverlayInviteDialog( CSteamID( lobbyId ) );
	}
}

/*
========================
Steam_ShowFriendsOverlay
========================
*/
void Steam_ShowFriendsOverlay()
{
	if( steamRunning )
	{
		SteamFriends()->ActivateGameOverlay( "Friends" );
	}
}

/*
========================
Steam_ShowUserOverlay
========================
*/
void Steam_ShowUserOverlay( uint64 steamId )
{
	if( steamRunning && steamId != 0 )
	{
		SteamFriends()->ActivateGameOverlayToUser( "steamid", CSteamID( steamId ) );
	}
}

/*
========================
Steam_ShowStoreOverlay
========================
*/
void Steam_ShowStoreOverlay( uint32 appId )
{
	if( steamRunning )
	{
		SteamFriends()->ActivateGameOverlayToStore( appId, k_EOverlayToStoreFlag_None );
	}
}

/*
========================
Steam_IsDlcInstalled
========================
*/
bool Steam_IsDlcInstalled( uint32 appId )
{
	if( !steamRunning || appId == 0 )
	{
		return true;
	}
	return SteamApps()->BIsDlcInstalled( appId );
}

/*
========================
Steam_IsMapAvailable
========================
*/
bool Steam_IsMapAvailable( const char* mapNameIn )
{
	if( !steamRunning || mapNameIn == NULL || mapNameIn[0] == '\0' )
	{
		return true;
	}

	idStr mapNameClean( mapNameIn );
	mapNameClean.BackSlashesToSlashes();
	mapNameClean.StripFileExtension();
	mapNameClean.StripLeading( "maps/" );
	const char* mapName = mapNameClean.c_str();

	const idStr list( steam_dlcMaps.GetString() );
	int pos = 0;
	while( pos < list.Length() )
	{
		while( pos < list.Length() && list[pos] == ' ' )
		{
			pos++;
		}
		int end = pos;
		while( end < list.Length() && list[end] != ' ' )
		{
			end++;
		}
		if( end > pos )
		{
			// "<prefix>:<app id>", the last colon splits the two
			const idStr pair = list.Mid( pos, end - pos );
			int colon = -1;
			for( int i = 0; i < pair.Length(); i++ )
			{
				if( pair[i] == ':' )
				{
					colon = i;
				}
			}
			if( colon > 0 )
			{
				idStr prefix = pair.Left( colon );
				prefix.BackSlashesToSlashes();
				prefix.StripLeading( "maps/" );
				if( prefix.Length() > 0 && idStr::Icmpn( mapName, prefix.c_str(), prefix.Length() ) == 0 )
				{
					return Steam_IsDlcInstalled( ( uint32 )strtoul( pair.c_str() + colon + 1, NULL, 10 ) );
				}
			}
		}
		pos = end;
	}

	return true;
}

/*
========================
Steam_StoreStats
========================
*/
void Steam_StoreStats()
{
	if( steamRunning && SteamUserStats() != NULL )
	{
		SteamUserStats()->StoreStats();
	}
}

/*
========================
connect_lobby
========================
*/
CONSOLE_COMMAND_SHIP( connect_lobby, "Join a Steam lobby: connect_lobby <lobby id>", NULL )
{
	if( args.Argc() < 2 )
	{
		idLib::Printf( "Usage: connect_lobby <lobby id>\n" );
		return;
	}
	if( !Steam_IsRunning() )
	{
		idLib::Printf( "Steam is not running\n" );
		return;
	}

	const uint64 lobbyId = ( uint64 )strtoull( args.Argv( 1 ), NULL, 10 );
	if( lobbyId == 0 )
	{
		idLib::Printf( "Invalid lobby id\n" );
		return;
	}
	if( steamCallbacks != NULL )
	{
		steamCallbacks->SetLobbyJoinRequest( lobbyId );
	}
}

CONSOLE_COMMAND( steam_status, "Prints the Steam state", NULL )
{
	if( !Steam_IsRunning() )
	{
		idLib::Printf( "Steam is not running\n" );
		return;
	}
	idLib::Printf( "Steam user: '%s' (%llu), logged on: %s, App ID: %u\n", Steam_GetPersonaName(), ( unsigned long long )Steam_GetLocalSteamId(),
				   SteamUser()->BLoggedOn() ? "yes" : "no", ( unsigned int )SteamUtils()->GetAppID() );
}
