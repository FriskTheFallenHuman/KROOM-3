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

/*
================================================================================================

Contains the windows implementation of the network session

================================================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "../../framework/Common_local.h"
#include "../sys_session_local.h"
#include "../sys_stats.h"
#include "../sys_savegame.h"
#include "../sys_lobby_backend_direct.h"
#include "../sys_voicechat.h"
#include "achievements.h"
#include "../sys_dedicated_server_search.h"

#ifdef USE_STEAMWORKS
	#include "../steam/steam_local.h"
	#include "../steam/steam_signin.h"
	#include "../steam/steam_achievements.h"
	#include "../steam/steam_voicechat.h"
	#include "../steam/steam_lobby_backend.h"
#endif
//#include "win_local.h"

/*
========================
Global variables
========================
*/

extern idCVar net_port;

class idLobbyToSessionCBLocal;

idSessionLocal sessionLocal;
idSession* session = &sessionLocal;

/*
========================
idLobbyToSessionCBLocal
========================
*/
class idLobbyToSessionCBLocal final : public idLobbyToSessionCB
{
public:
	idLobbyToSessionCBLocal( idSessionLocal* owner_ ) : owner( owner_ ) { }

	bool CanJoinLocalHost() const override
	{
		owner->EnsurePort();
		return owner->canJoinLocalHost;
	}
	class idLobbyBackend* GetLobbyBackend( idLobbyBackend::lobbyBackendType_t type ) const override
	{
		return owner->lobbyBackends[ type ];
	}

private:
	idSessionLocal* 				owner;
};

idLobbyToSessionCBLocal lobbyToSessionCBLocal( &sessionLocal );
idLobbyToSessionCB* lobbyToSessionCB = &lobbyToSessionCBLocal;

/*
========================
idSessionLocal::Initialize
========================
*/
void idSessionLocal::Initialize()
{
#ifdef USE_STEAMWORKS
	if( Steam_IsRunning() )
	{
		assert( signInManager->GetNumLocalUsers() == 0 );
		delete signInManager;
		signInManager = new( TAG_SYSTEM ) idSignInManagerSteam;

		delete voiceChat;
		voiceChat = new( TAG_SYSTEM ) idVoiceChatMgrSteam();
	}
#endif

	// The shipping path doesn't load title storage
	// Instead, we inject values through code which is protected through steam DRM
	titleStorageVars.Set( "MAX_PLAYERS_ALLOWED", "8" );
	titleStorageLoaded = true;

	GetPartyLobby().Initialize( idLobby::TYPE_PARTY, sessionCallbacks );
	GetGameLobby().Initialize( idLobby::TYPE_GAME, sessionCallbacks );
	GetGameStateLobby().Initialize( idLobby::TYPE_GAME_STATE, sessionCallbacks );

#ifdef USE_STEAMWORKS
	if( Steam_IsRunning() )
	{
		achievementSystem = new( TAG_SYSTEM ) idAchievementSystemSteam();
	}
	else
#endif
	{
		achievementSystem = new( TAG_SYSTEM ) idAchievementSystemLocal();
	}
	achievementSystem->Init();
}

/*
========================
idSessionLocal::Shutdown
========================
*/
void idSessionLocal::Shutdown()
{
	NET_VERBOSE_PRINT( "NET: Shutdown\n" );

	// shut any services we currently have
	DestroyServices();

	MoveToMainMenu();

	// Wait until we fully shutdown
	while( localState != STATE_IDLE )
	{
		Pump();
	}

	if( achievementSystem != NULL )
	{
		achievementSystem->Shutdown();
		delete achievementSystem;
		achievementSystem = NULL;
	}
}

/*
========================
idSessionLocal::InitializeSoundRelatedSystems
========================
*/
void idSessionLocal::InitializeSoundRelatedSystems()
{
	if( voiceChat != NULL )
	{
		voiceChat->Init( NULL );
	}
}

/*
========================
idSessionLocal::ShutdownSoundRelatedSystems
========================
*/
void idSessionLocal::ShutdownSoundRelatedSystems()
{
	if( voiceChat != NULL )
	{
		voiceChat->Shutdown();
	}
}

/*
========================
idSessionLocal::InviteFriends
========================
*/
void idSessionLocal::InviteFriends()
{
#ifdef USE_STEAMWORKS
	// Friends are invited to the lobby the platform shows them, the party lobby when there is one
	idLobby* lobby = GetActivePlatformLobby();
	if( lobby != NULL && lobby->lobbyBackend != NULL )
	{
		Steam_ShowInviteDialog( lobby->lobbyBackend->GetConnectInfo().platformLobbyId );
	}
#endif
}

/*
========================
idSessionLocal::InviteParty
========================
*/
void idSessionLocal::InviteParty()
{
	// Some platforms have no party of their own, inviting the party is inviting friends to the lobby
	InviteFriends();
}

/*
========================
idSessionLocal::ShowPartySessions
========================
*/
void idSessionLocal::ShowPartySessions()
{
#ifdef USE_STEAMWORKS
	// the sessions of the friends are their lobbies, the friends list has the "Join Game" button
	Steam_ShowFriendsOverlay();
#endif
}

/*
========================
idSessionLocal::ShowOnlineSignin
========================
*/
void idSessionLocal::ShowOnlineSignin()
{
}

/*
========================
idSessionLocal::ListServers
========================
*/
void idSessionLocal::ListServers( const idCallback& callback )
{
#ifdef USE_STEAMWORKS
	if( Steam_UseLobbies() )
	{
		Steam_BrowserRefresh( callback );
		return;
	}
#endif

	dedicatedServerSearch->StartSearch( callback );

	// TODO: Do i need this commented?
	//idCallback* localCallBack = callback.Clone();
	//if( localCallBack )
	//{
	//	localCallBack->Call();
	//	delete localCallBack;
	//	localCallBack = NULL;
	//}
}

/*
========================
idSessionLocal::CancelListServers
========================
*/
void idSessionLocal::CancelListServers()
{
#ifdef USE_STEAMWORKS
	Steam_BrowserCancel();
#endif
}

/*
========================
idSessionLocal::NumServers
========================
*/
int idSessionLocal::NumServers() const
{
#ifdef USE_STEAMWORKS
	if( Steam_UseLobbies() )
	{
		return Steam_BrowserNumServers();
	}
#endif
	return dedicatedServerSearch->NumServers();
}

/*
========================
idSessionLocal::ServerInfo
========================
*/
const serverInfo_t* idSessionLocal::ServerInfo( int i ) const
{
#ifdef USE_STEAMWORKS
	if( Steam_UseLobbies() )
	{
		return Steam_BrowserServerInfo( i );
	}
#endif
	return dedicatedServerSearch->DescribeServerAtIndex( i );
}

/*
========================
idSessionLocal::ConnectToServer
========================
*/
void idSessionLocal::ConnectToServer( int i )
{
#ifdef USE_STEAMWORKS
	if( Steam_UseLobbies() )
	{
		const uint64 lobbyId = Steam_BrowserLobbyId( i );
		if( lobbyId != 0 )
		{
			lobbyConnectInfo_t connectInfo;
			connectInfo.platformLobbyId = lobbyId;
			ConnectAndMoveToLobby( GetPartyLobby(), connectInfo, false );
		}
		return;
	}
#endif

	lobbyConnectInfo_t serverAddr;
	dedicatedServerSearch->GetAddrAtIndex( serverAddr.netAddr, i );

	ConnectAndMoveToLobby( GetPartyLobby(), serverAddr, false );
}

/*
========================
idSessionLocal::Connect_f
========================
*/
void idSessionLocal::Connect_f( const idCmdArgs& args )
{
	if( args.Argc() < 2 )
	{
		const char* usage =
			"Connect to the specified IP and port. Usage:\n"
			"\n"
			" connect <ip>\n"
			" connect <ip> <port>\n"
			"\n"
			"If <port> isn't specified, the default port (%s) is used.\n";
		idLib::Printf( usage, net_port.GetDefaultString() );
		return;
	}

	lobbyConnectInfo_t connectInfo;
	if( !Sys_StringToNetAdr( args.Argv( 1 ), &connectInfo.netAddr, true ) )
	{
		idLib::Printf( "Invalid IP.\n" );
		return;
	}
	if( args.Argc() > 2 )
	{
		int port = atoi( args.Argv( 2 ) );
		if( port < 0 || port > 65535 )
		{
			idLib::Printf( "Invalid port.\n" );
			return;
		}
		connectInfo.netAddr.port = ( unsigned short )port;
	}
	else
	{
		connectInfo.netAddr.port = ( unsigned short )atoi( net_port.GetDefaultString() );
	}
	idLib::Printf( "Connecting to %s\n", Sys_NetAdrToString( connectInfo.netAddr ) );

	Cancel();
	common->SetServerDedicated( false );

	if( signInManager->GetMasterLocalUser() == NULL )
	{
		signInManager->RegisterLocalUser( 0 );
	}

	ConnectAndMoveToLobby( GetPartyLobby(), connectInfo, false );
}

/*
========================
void Connect_f
========================
*/
CONSOLE_COMMAND_SHIP( connect, "Connect to the specified IP and port", NULL )
{
	sessionLocal.Connect_f( args );
}

/*
========================
idSessionLocal::ShowServerGamerCardUI
========================
*/
void idSessionLocal::ShowServerGamerCardUI( int i )
{
#ifdef USE_STEAMWORKS
	if( Steam_UseLobbies() )
	{
		Steam_ShowUserOverlay( Steam_BrowserHostId( i ) );
	}
#endif
}

/*
========================
idSessionLocal::ShowLobbyUserGamerCardUI(
========================
*/
void idSessionLocal::ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID )
{
#ifdef USE_STEAMWORKS
	idLobby* lobby = GetActivePlatformLobby();
	if( lobby != NULL )
	{
		const lobbyUser_t* user = lobby->GetLobbyUserByID( lobbyUserID );
		if( user != NULL )
		{
			Steam_ShowUserOverlay( user->platformId );		// 0 for a user without a Steam account, ignored
		}
	}
#endif
}

/*
========================
idSessionLocal::ProcessInputEvent
========================
*/
bool idSessionLocal::ProcessInputEvent( const sysEvent_t* ev )
{
	if( GetSignInManager().ProcessInputEvent( ev ) )
	{
		return true;
	}
	return false;
}

/*
========================
idSessionLocal::IsSystemUIShowing
========================
*/
bool idSessionLocal::IsSystemUIShowing() const
{
	// DG: pausing here when window is out of focus like originally done on windows is hacky
	// it's done with com_pause now.
	return isSysUIShowing;
}

/*
========================
idSessionLocal::SetSystemUIShowing
========================
*/
void idSessionLocal::SetSystemUIShowing( bool show )
{
	isSysUIShowing = show;
}

/*
========================
idSessionLocal::HandleServerQueryRequest
========================
*/
void idSessionLocal::HandleServerQueryRequest( lobbyAddress_t& remoteAddr, idBitMsg& msg, int msgType )
{
	NET_VERBOSE_PRINT( "HandleServerQueryRequest from %s\n", remoteAddr.ToString() );
	HandleDedicatedServerQueryRequest( remoteAddr, msg, msgType );
}

/*
========================
idSessionLocal::HandleServerQueryAck
========================
*/
void idSessionLocal::HandleServerQueryAck( lobbyAddress_t& remoteAddr, idBitMsg& msg )
{
	NET_VERBOSE_PRINT( "HandleServerQueryAck from %s\n", remoteAddr.ToString() );
	HandleDedicatedServerQueryAck( remoteAddr, msg );
}

/*
========================
idSessionLocal::HandleBootableInvite
========================
*/
void idSessionLocal::HandleBootableInvite( int64 lobbyId )
{
#ifdef USE_STEAMWORKS
	if( lobbyId == 0 || !Steam_UseLobbies() )
	{
		return;
	}

	// The invite can arrive before the player pressed start, register the controller like the connect command does
	if( signInManager->GetMasterLocalUser() == NULL )
	{
		signInManager->RegisterLocalUser( 0 );
	}

	idLocalUser* masterLocalUser = signInManager->GetMasterLocalUser();
	if( masterLocalUser == NULL )
	{
		return;
	}

	// UpdatePendingInvite does the join as soon as the session is back at the main menu
	pendingInviteConnectInfo = lobbyConnectInfo_t();
	pendingInviteConnectInfo.platformLobbyId = ( uint64 )lobbyId;
	pendingInviteDevice = masterLocalUser->GetInputDevice();
	pendingInviteMode = PENDING_INVITE_WAITING;
	pendingInviteStartTime = Sys_Milliseconds();
#endif
}

/*
========================
idSessionLocal::LeaderboardFindDef
========================
*/
const leaderboardDefinition_t* idSessionLocal::LeaderboardFindDef( int id )
{
	return Sys_FindLeaderboardDef( id );
}

/*
========================
idSessionLocal::LeaderboardCreateDef
========================
*/
leaderboardDefinition_t* idSessionLocal::LeaderboardCreateDef( int id_, int numColumns_, const columnDef_t* columnDefs_,
		int rankOrder_, bool supportsAttachments_, bool checkAgainstCurrent_ )
{
	return Sys_CreateLeaderboardDef( id_, numColumns_, columnDefs_, ( rankOrder_t )rankOrder_, supportsAttachments_, checkAgainstCurrent_ );
}

/*
========================
idSessionLocal::LeaderboardDestroyDefs
========================
*/
void idSessionLocal::LeaderboardDestroyDefs()
{
	Sys_DestroyLeaderboardDefs();
}

/*
========================
idSessionLocal::LeaderboardUpload
========================
*/
void idSessionLocal::LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t* leaderboard, const column_t* stats, const idFile_Memory* attachment )
{
#ifdef USE_STEAMWORKS
	if( !Steam_IsRunning() || leaderboard == NULL || stats == NULL )
	{
		return;
	}

	idLobby& lobby = GetActingGameStateLobby();
	const int userIndex = lobby.GetLobbyUserIndexByID( lobbyUserID );
	if( userIndex < 0 || lobby.IsLobbyUserDisconnected( userIndex ) )
	{
		return;
	}

	if( !lobby.IsSessionUserIndexLocal( userIndex ) )
	{
		// Steam only takes a score for the account that is signed in, the machine of the user submits it
		if( lobby.IsHost() )
		{
			SendLeaderboardStatsToPlayer( lobbyUserID, leaderboard, stats );
		}
		return;
	}

	const idLocalUser* localUser = signInManager->GetLocalUserByHandle( lobbyUserID.GetLocalUserHandle() );
	if( localUser != NULL && localUser->GetPlatformId() != 0 )
	{
		Steam_LeaderboardUpload( leaderboard, stats );
	}
#endif
}

/*
========================
idSessionLocal::LeaderboardFlush
========================
*/
void idSessionLocal::LeaderboardFlush()
{
}

/*
========================
idSessionLocal::LeaderboardDownload
========================
*/
void idSessionLocal::LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int startingRank, int numRows, const idLeaderboardCallback& callback )
{
#ifdef USE_STEAMWORKS
	if( Steam_IsRunning() )
	{
		Steam_LeaderboardDownload( leaderboard, startingRank, numRows, callback );
	}
#endif
}

/*
========================
idSessionLocal::LeaderboardDownloadAttachment
========================
*/
void idSessionLocal::LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int64 attachmentID )
{
}

/*
========================
idSessionLocal::EnsurePort
========================
*/
void idSessionLocal::EnsurePort()
{
	// Init the port using reqular sockets
	if( port.IsOpen() )
	{
		return;		// Already initialized
	}

	if( port.InitPort( net_port.GetInteger(), false ) )
	{
		// TODO: what about canJoinLocalHost when running two instances with different net_port values?
		canJoinLocalHost = false;
	}
	else
	{
		// Assume this is another instantiation on the same machine, and just init using any available port
		port.InitPort( PORT_ANY, false );
		canJoinLocalHost = true;
	}
}

/*
========================
idSessionLocal::GetPort
========================
*/
idNetSessionPort& idSessionLocal::GetPort( bool dedicated )
{
	EnsurePort();
	return port;
}

/*
========================
UsePlatformLobby
========================
*/
static bool UsePlatformLobby( int matchFlags )
{
#ifdef USE_STEAMWORKS
	return MatchTypeIsOnline( ( uint8 )matchFlags ) && Steam_UseLobbies();
#else
	return false;
#endif
}

/*
========================
idSessionLocal::CreateLobbyBackend
========================
*/
idLobbyBackend* idSessionLocal::CreateLobbyBackend( const idMatchParameters& p, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType, UsePlatformLobby( p.matchFlags ) );
	lobbyBackend->StartHosting( p, skillLevel, lobbyType );
	return lobbyBackend;
}

/*
========================
idSessionLocal::FindLobbyBackend
========================
*/
idLobbyBackend* idSessionLocal::FindLobbyBackend( const idMatchParameters& p, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType, UsePlatformLobby( p.matchFlags ) );
	lobbyBackend->StartFinding( p, numPartyUsers, skillLevel );
	return lobbyBackend;
}

/*
========================
idSessionLocal::JoinFromConnectInfo
========================
*/
idLobbyBackend* idSessionLocal::JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	// A connect info with a platform lobby id can only be joined through the platform
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType, connectInfo.platformLobbyId != 0 && UsePlatformLobby( MATCH_ONLINE ) );
	lobbyBackend->JoinFromConnectInfo( connectInfo );
	return lobbyBackend;
}

/*
========================
idSessionLocal::DestroyLobbyBackend
========================
*/
void idSessionLocal::DestroyLobbyBackend( idLobbyBackend* lobbyBackend )
{
	assert( lobbyBackend != NULL );
	assert( lobbyBackends[lobbyBackend->GetLobbyType()] == lobbyBackend );

	lobbyBackends[lobbyBackend->GetLobbyType()] = NULL;

	lobbyBackend->Shutdown();
	delete lobbyBackend;
}

/*
========================
idSessionLocal::PumpLobbies
========================
*/
void idSessionLocal::PumpLobbies()
{
	assert( lobbyBackends[idLobbyBackend::TYPE_PARTY] == NULL || lobbyBackends[idLobbyBackend::TYPE_PARTY]->GetLobbyType() == idLobbyBackend::TYPE_PARTY );
	assert( lobbyBackends[idLobbyBackend::TYPE_GAME] == NULL || lobbyBackends[idLobbyBackend::TYPE_GAME]->GetLobbyType() == idLobbyBackend::TYPE_GAME );
	assert( lobbyBackends[idLobbyBackend::TYPE_GAME_STATE] == NULL || lobbyBackends[idLobbyBackend::TYPE_GAME_STATE]->GetLobbyType() == idLobbyBackend::TYPE_GAME_STATE );

	// Pump lobbyBackends
	for( int i = 0; i < lobbyBackends.Num(); i++ )
	{
		if( lobbyBackends[i] != NULL )
		{
			lobbyBackends[i]->Pump();
		}
	}
}

/*
========================
idSessionLocal::CreateLobbyInternal
========================
*/
idLobbyBackend* idSessionLocal::CreateLobbyInternal( idLobbyBackend::lobbyBackendType_t lobbyType, bool usePlatform )
{
	EnsurePort();

	idLobbyBackend* lobbyBackend = NULL;

#ifdef USE_STEAMWORKS
	if( usePlatform )
	{
		lobbyBackend = new( TAG_NETWORKING ) idLobbyBackendSteam();
	}
#endif

	if( lobbyBackend == NULL )
	{
		lobbyBackend = new( TAG_NETWORKING ) idLobbyBackendDirect();
	}

	lobbyBackend->SetLobbyType( lobbyType );

	assert( lobbyBackends[lobbyType] == NULL );
	lobbyBackends[lobbyType] = lobbyBackend;

	return lobbyBackend;
}

/*
========================
idSessionLocal::GetLobbyAddressFromNetAddress
========================
*/
bool idSessionLocal::GetLobbyAddressFromNetAddress( const netadr_t& netAddr, lobbyAddress_t& outAddr ) const
{
	return false;
}

/*
========================
idSessionLocal::GetNetAddressFromLobbyAddress
========================
*/
bool idSessionLocal::GetNetAddressFromLobbyAddress( const lobbyAddress_t& lobbyAddress, netadr_t& outNetAddr ) const
{
	return false;
}
