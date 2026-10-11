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

#ifndef __STEAM_LOCAL_H__
#define __STEAM_LOCAL_H__

#include <steam/steam_api.h>

#ifndef STEAM_APP_ID
	#define STEAM_APP_ID 208200
#endif

class lobbyAddress_t;
class idCallback;
struct serverInfo_t;

extern idCVar steam_verbose;
#define STEAM_VERBOSE_PRINT if ( steam_verbose.GetBool() ) idLib::Printf

/*
================================================
steamInitResult_t
================================================
*/
enum steamInitResult_t
{
	STEAM_INIT_OK,			// Steam is up.
	STEAM_INIT_DISABLED,	// steam_enable is 0, we are running offline.
	STEAM_INIT_RESTART,		// the game was not started through Steam and Steam was asked to relaunch it, quit.
	STEAM_INIT_FAILED,		// Steam is not running or too old, we are running offline.
};

steamInitResult_t Steam_Init();
bool Steam_IsRunning();
bool Steam_UseLobbies();
void Steam_RunCallbacks();
void Steam_Shutdown();

uint64 Steam_GetLocalSteamId();
const char* Steam_GetPersonaName();

// Lobby the player asked to join through the overlay
uint64 Steam_PopLobbyJoinRequest();

// Called from idSessionLocal::Pump, only talks to Steam when something changed.
struct steamPresence_t
{
	int state;				// idSession::sessionState_t
	uint64 partyLobbyId;		// Steam id of the party lobby we host or are in, 0 if none
	int numPlayers;
	int maxPlayers;
};
void Steam_UpdatePresence( const steamPresence_t& presence );

ID_ENGINE_API void Steam_ShowInviteDialog( uint64 lobbyId );
ID_ENGINE_API void Steam_ShowUserOverlay( uint64 steamId );
ID_ENGINE_API void Steam_ShowFriendsOverlay();
ID_ENGINE_API void Steam_ShowStoreOverlay( uint32 appId );

// DLC. Both return "available" when Steam is not running, so an offline build never gates content.
bool Steam_IsDlcInstalled( uint32 appId );
bool Steam_IsMapAvailable( const char* mapName );	// map name, map file or the name stored in a save

// Pushes locally changed stats and achievements to Steam.
void Steam_StoreStats();

// Session id of the user, stable for one Steam account.
int Steam_HashSteamId( uint64 steamId );

// What the host says about its game.
struct steamBrowseListing_t
{
	bool listed;				// online, not private, not invite only, joinable, free slots
	int gameMode;
	int gameMap;
	int numPlayers;
	int maxPlayers;
	bool joinable;
};
void Steam_SetBrowseListing( const steamBrowseListing_t& listing );

void Steam_BrowserRefresh( const idCallback& callback );
void Steam_BrowserCancel();
int Steam_BrowserNumServers();
const serverInfo_t*	Steam_BrowserServerInfo( int i );
uint64 Steam_BrowserLobbyId( int i );		// party lobby of the host, what ConnectToServer() joins
uint64 Steam_BrowserHostId( int i );		// Steam id of the host
void Steam_BrowserShutdown();

bool Steam_NetReadPacket( lobbyAddress_t& from, void* data, int& size, int maxSize );
bool Steam_NetSendPacket( const lobbyAddress_t& to, const void* data, int size );
void Steam_NetShutdown();

// Steam leaderboards are looked up by name, the engine ones are created on demand as STEAM_LEADERBOARD_PREFIX + id
#define STEAM_LEADERBOARD_PREFIX		"lb_"

struct leaderboardDefinition_t;
struct column_t;
class idLeaderboardCallback;

// Scores are always submitted for the local master user, Steam can not submit for somebody else.
void Steam_LeaderboardUpload( const leaderboardDefinition_t* def, const column_t* stats );

// startingRank: 0 = rows around the local user, -1 = friends, otherwise the global rank of the first row (1 based).
// The callback is cloned and called from Steam_RunCallbacks, never from inside this function.
void Steam_LeaderboardDownload( const leaderboardDefinition_t* def, int startingRank, int numRows, const idLeaderboardCallback& callback );

void Steam_LeaderboardPump();		// called by Steam_RunCallbacks
void Steam_LeaderboardShutdown();	// called by Steam_Shutdown

// True if the given account is a member of a Steam lobby we are in.
bool Steam_IsLobbyMember( uint64 steamId );

#endif // __STEAM_LOCAL_H__
