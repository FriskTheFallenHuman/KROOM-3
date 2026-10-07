/*
================================================================================================
CONFIDENTIAL AND PROPRIETARY INFORMATION/NOT FOR DISCLOSURE WITHOUT WRITTEN PERMISSION
Copyright 2010 id Software LLC, a ZeniMax Media company. All Rights Reserved.
================================================================================================
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
	// The shipping path doesn't load title storage
	// Instead, we inject values through code which is protected through steam DRM
	titleStorageVars.Set( "MAX_PLAYERS_ALLOWED", "8" );
	titleStorageLoaded = true;

	GetPartyLobby().Initialize( idLobby::TYPE_PARTY, sessionCallbacks );
	GetGameLobby().Initialize( idLobby::TYPE_GAME, sessionCallbacks );
	GetGameStateLobby().Initialize( idLobby::TYPE_GAME_STATE, sessionCallbacks );

	achievementSystem = new( TAG_SYSTEM ) idAchievementSystemLocal();
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
}

/*
========================
idSessionLocal::InviteParty
========================
*/
void idSessionLocal::InviteParty()
{
}

/*
========================
idSessionLocal::ShowPartySessions
========================
*/
void idSessionLocal::ShowPartySessions()
{
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
}

/*
========================
idSessionLocal::NumServers
========================
*/
int idSessionLocal::NumServers() const
{
	return dedicatedServerSearch->NumServers();
}

/*
========================
idSessionLocal::ServerInfo
========================
*/
const serverInfo_t* idSessionLocal::ServerInfo( int i ) const
{
	return dedicatedServerSearch->DescribeServerAtIndex( i );
}

/*
========================
idSessionLocal::ConnectToServer
========================
*/
void idSessionLocal::ConnectToServer( int i )
{
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
}

/*
========================
idSessionLocal::ShowLobbyUserGamerCardUI(
========================
*/
void idSessionLocal::ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID )
{
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
idSessionLocal::CreateLobbyBackend
========================
*/
idLobbyBackend* idSessionLocal::CreateLobbyBackend( const idMatchParameters& p, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType )
{
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType );
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
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType );
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
	idLobbyBackend* lobbyBackend = CreateLobbyInternal( lobbyType );
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
idLobbyBackend* idSessionLocal::CreateLobbyInternal( idLobbyBackend::lobbyBackendType_t lobbyType )
{
	EnsurePort();
	idLobbyBackend* lobbyBackend = new( TAG_NETWORKING ) idLobbyBackendDirect();

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
