/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

#ifndef __ASYNCNETWORK_H__
#define __ASYNCNETWORK_H__

#include "MsgChannel.h"
#include "AsyncServer.h"
#include "ServerScan.h"
#include "AsyncClient.h"

/*
===============================================================================

  Asynchronous Networking.

===============================================================================
*/


// unreliable server -> client messages
enum
{
	SERVER_UNRELIABLE_MESSAGE_EMPTY = 0,
	SERVER_UNRELIABLE_MESSAGE_PING,
	SERVER_UNRELIABLE_MESSAGE_GAMEINIT,
	SERVER_UNRELIABLE_MESSAGE_SNAPSHOT
};

// reliable server -> client messages
enum
{
	SERVER_RELIABLE_MESSAGE_PURE = 0,
	SERVER_RELIABLE_MESSAGE_RELOAD,
	SERVER_RELIABLE_MESSAGE_CLIENTINFO,
	SERVER_RELIABLE_MESSAGE_SYNCEDCVARS,
	SERVER_RELIABLE_MESSAGE_PRINT,
	SERVER_RELIABLE_MESSAGE_DISCONNECT,
	SERVER_RELIABLE_MESSAGE_APPLYSNAPSHOT,
	SERVER_RELIABLE_MESSAGE_GAME,
	SERVER_RELIABLE_MESSAGE_ENTERGAME
};

// unreliable client -> server messages
enum
{
	CLIENT_UNRELIABLE_MESSAGE_EMPTY = 0,
	CLIENT_UNRELIABLE_MESSAGE_PINGRESPONSE,
	CLIENT_UNRELIABLE_MESSAGE_USERCMD
};

// reliable client -> server messages
enum
{
	CLIENT_RELIABLE_MESSAGE_PURE = 0,
	CLIENT_RELIABLE_MESSAGE_CLIENTINFO,
	CLIENT_RELIABLE_MESSAGE_PRINT,
	CLIENT_RELIABLE_MESSAGE_DISCONNECT,
	CLIENT_RELIABLE_MESSAGE_GAME
};

// server print messages
enum
{
	SERVER_PRINT_MISC = 0,
	SERVER_PRINT_BADPROTOCOL,
	SERVER_PRINT_RCON,
	SERVER_PRINT_GAMEDENY,
	SERVER_PRINT_BADCHALLENGE
};

enum
{
	SERVER_DL_REDIRECT = 1,
	SERVER_DL_LIST,
	SERVER_DL_NONE
};

enum
{
	SERVER_PAK_NO = 0,
	SERVER_PAK_YES,
	SERVER_PAK_END
};

typedef struct master_s
{
	idCVar* 		var;
	netadr_t		address;
	bool			resolved;
} master_t;


class idAsyncNetwork
{
public:
	idAsyncNetwork();

	static void				Init();
	static void				Shutdown();
	static bool				IsActive()
	{
		return ( server.IsActive() || client.IsActive() );
	}
	static void				RunFrame();

	static void				WriteUserCmdDelta( idBitMsg& msg, const usercmd_t& cmd, const usercmd_t* base );
	static void				ReadUserCmdDelta( const idBitMsg& msg, usercmd_t& cmd, const usercmd_t* base );

	static bool				DuplicateUsercmd( const usercmd_t& previousUserCmd, usercmd_t& currentUserCmd, int frame, int time );
	static bool				UsercmdInputChanged( const usercmd_t& previousUserCmd, const usercmd_t& currentUserCmd );

	// returns true if the corresponding master is set to something (and could be resolved)
	static bool				GetMasterAddress( int index, netadr_t& adr );
	// get the hardcoded idnet master, equivalent to GetMasterAddress( 0, .. )
	static netadr_t			GetMasterAddress();

	static void				GetNETServers( );

	static void				ExecuteSessionCommand( const char* sessCmd );

	static idAsyncServer	server;
	static idAsyncClient	client;

	static idCVar			verbose;						// verbose output
	static idCVar			allowCheats;					// allow cheats
	static idCVar			serverDedicated;				// if set run a dedicated server
	static idCVar			serverSnapshotDelay;			// number of milliseconds between snapshots
	static idCVar			serverMaxClientRate;			// maximum outgoing rate to clients
	static idCVar			clientMaxRate;					// maximum rate from server requested by client
	static idCVar			serverMaxUsercmdRelay;			// maximum number of usercmds relayed to other clients
	static idCVar			serverZombieTimeout;			// time out in seconds for zombie clients
	static idCVar			serverClientTimeout;			// time out in seconds for connected clients
	static idCVar			clientServerTimeout;			// time out in seconds for server
	static idCVar			serverDrawClient;				// the server draws the view of this client
	static idCVar			serverRemoteConsolePassword;	// remote console password
	static idCVar			clientPrediction;				// how many additional milliseconds the clients runs ahead
	static idCVar			clientMaxPrediction;			// max milliseconds into the future a client can run prediction
	static idCVar			clientUsercmdBackup;			// how many usercmds the client sends from previous frames
	static idCVar			clientRemoteConsoleAddress;		// remote console address
	static idCVar			clientRemoteConsolePassword;	// remote console password
	static idCVar			master0;						// idnet master server
	static idCVar			master1;						// 1st master server
	static idCVar			master2;						// 2nd master server
	static idCVar			master3;						// 3rd master server
	static idCVar			master4;						// 4th master server
	static idCVar			LANServer;						// LAN mode
	static idCVar			serverReloadEngine;				// reload engine on map change instead of growing the referenced paks
	static idCVar			serverAllowServerMod;			// let a pure server start with a different game code than what is referenced in game code
	static idCVar			idleServer;						// serverinfo reply, indicates all clients are idle
	static idCVar			clientDownload;					// preferred download policy

	// same message used for offline check and network reply
	static void				BuildInvalidKeyMsg( idStr& msg, bool valid[ 2 ] );

private:
	static int				realTime;
	static master_t			masters[ MAX_MASTER_SERVERS];	// master1 etc.

	static void				SpawnServer_f( const idCmdArgs& args );
	static void				NextMap_f( const idCmdArgs& args );
	static void				Connect_f( const idCmdArgs& args );
	static void				Reconnect_f( const idCmdArgs& args );
	static void				GetServerInfo_f( const idCmdArgs& args );
	static void				GetLANServers_f( const idCmdArgs& args );
	static void				ListServers_f( const idCmdArgs& args );
	static void				RemoteConsole_f( const idCmdArgs& args );
	static void				Heartbeat_f( const idCmdArgs& args );
	static void				Kick_f( const idCmdArgs& args );
	static void				CheckNewVersion_f( const idCmdArgs& args );
	static void				UpdateUI_f( const idCmdArgs& args );
};

#endif /* !__ASYNCNETWORK_H__ */
