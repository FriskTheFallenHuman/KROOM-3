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


#undef private
#undef protected

// DG: achievements and signin is the same on windows, linux => put them in common dir
#include "common/achievements.h"
#include "common/signin.h"
// DG end

#include "sys_lobby_backend.h"
#include "sys_lobby.h"

class idSaveGameProcessorNextMap;
class idSaveGameProcessorSaveGame;
class idSaveGameProcessorLoadGame;
class idSaveGameProcessorDelete;
class idSaveGameProcessorEnumerateGames;

/*
================================================
idLobbyStub
================================================
*/
class idLobbyStub : public idLobbyBase
{
public:
	virtual bool						IsHost() const
	{
		return false;
	}
	virtual bool						IsPeer() const
	{
		return false;
	}
	virtual bool						HasActivePeers() const
	{
		return false;
	}
	virtual int							GetNumLobbyUsers() const
	{
		return 0;
	}
	virtual int							GetNumActiveLobbyUsers() const
	{
		return 0;
	}
	virtual bool						IsLobbyUserConnected( int index ) const
	{
		return false;
	}

	virtual lobbyUserID_t				GetLobbyUserIdByOrdinal( int userIndex ) const
	{
		return lobbyUserID_t();
	}
	virtual	int							GetLobbyUserIndexFromLobbyUserID( lobbyUserID_t lobbyUserID ) const
	{
		return -1;
	}

	virtual void						SendReliable( int type, idBitMsg& msg, bool callReceiveReliable = true, peerMask_t sessionUserMask = MAX_UNSIGNED_TYPE( peerMask_t ) ) {}
	virtual void						SendReliableToLobbyUser( lobbyUserID_t lobbyUserID, int type, idBitMsg& msg ) {}
	virtual void						SendReliableToHost( int type, idBitMsg& msg ) {}

	virtual const char* 				GetLobbyUserName( lobbyUserID_t lobbyUserID ) const
	{
		return "INVALID";
	}
	virtual void						KickLobbyUser( lobbyUserID_t lobbyUserID ) {}
	virtual bool						IsLobbyUserValid( lobbyUserID_t lobbyUserID ) const
	{
		return false;
	}
	virtual bool						IsLobbyUserLoaded( lobbyUserID_t lobbyUserID ) const
	{
		return false;
	}
	virtual bool						LobbyUserHasFirstFullSnap( lobbyUserID_t lobbyUserID ) const
	{
		return false;
	}
	virtual void						EnableSnapshotsForLobbyUser( lobbyUserID_t lobbyUserID ) {}

	virtual int							GetLobbyUserSkinIndex( lobbyUserID_t lobbyUserID ) const
	{
		return 0;
	}
	virtual bool						GetLobbyUserWeaponAutoReload( lobbyUserID_t lobbyUserID ) const
	{
		return false;
	}
	virtual bool						GetLobbyUserWeaponAutoSwitch( lobbyUserID_t lobbyUserID ) const
	{
		return false;
	}
	virtual int							GetLobbyUserLevel( lobbyUserID_t lobbyUserID ) const
	{
		return 0;
	}
	virtual int							GetLobbyUserQoS( lobbyUserID_t lobbyUserID ) const
	{
		return 0;
	}
	virtual int							GetLobbyUserTeam( lobbyUserID_t lobbyUserID ) const
	{
		return 0;
	}
	virtual bool						SetLobbyUserTeam( lobbyUserID_t lobbyUserID, int teamNumber )
	{
		return false;
	}
	virtual int							GetLobbyUserPartyToken( lobbyUserID_t lobbyUserID ) const
	{
		return 0;
	}
	virtual idPlayerProfile* 			GetProfileFromLobbyUser( lobbyUserID_t lobbyUserID )
	{
		return NULL;
	}
	virtual idLocalUser* 				GetLocalUserFromLobbyUser( lobbyUserID_t lobbyUserID )
	{
		return NULL;
	}
	virtual int							GetNumLobbyUsersOnTeam( int teamNumber ) const
	{
		return 0;
	}

	virtual int							PeerIndexFromLobbyUser( lobbyUserID_t lobbyUserID ) const
	{
		return -1;
	}

	virtual int							GetPeerTimeSinceLastPacket( int peerIndex ) const
	{
		return 0;
	}
	virtual int							PeerIndexForHost() const
	{
		return -1;
	}

	virtual lobbyUserID_t				AllocLobbyUserSlotForBot( const char* botName )
	{
		return lobbyUserID_t();
	}
	virtual void						RemoveBotFromLobbyUserList( lobbyUserID_t lobbyUserID ) {}
	virtual bool						GetLobbyUserIsBot( lobbyUserID_t lobbyUserID ) const
	{
		return false;
	}

	virtual const char* 				GetHostUserName() const
	{
		return "INVALID";
	}
	virtual const idMatchParameters& 	GetMatchParms() const
	{
		return fakeParms;
	}
	virtual bool						IsLobbyFull() const
	{
		return false;
	}

	virtual bool						EnsureAllPeersHaveBaseState()
	{
		return false;
	}
	virtual bool						AllPeersInGame() const
	{
		return false;
	}
	virtual int							GetNumConnectedPeers() const
	{
		return 0;
	}
	virtual int							GetNumConnectedPeersInGame() const
	{
		return 0;
	}
	virtual int							PeerIndexOnHost() const
	{
		return -1;
	}
	virtual bool						IsPeerDisconnected( int peerIndex ) const
	{
		return false;
	}

	virtual bool						AllPeersHaveStaleSnapObj( int objId )
	{
		return false;
	}
	virtual bool						AllPeersHaveExpectedSnapObj( int objId )
	{
		return false;
	}
	virtual void						RefreshSnapObj( int objId ) {}
	virtual void						MarkSnapObjDeleted( int objId ) {}
	virtual void						AddSnapObjTemplate( int objID, idBitMsg& msg ) {}

	virtual void						DrawDebugNetworkHUD() const {}
	virtual void						DrawDebugNetworkHUD2() const {}
	virtual void						DrawDebugNetworkHUD_ServerSnapshotMetrics( bool draw ) {}
private:
	idMatchParameters					fakeParms;
};

/*
================================================
idSessionLocal
================================================
*/
class idLobbyToSessionCBLocal;

class idSessionLocal final : public idSession
{
	friend class idLeaderboards;
	friend class idStatsSession;
	friend class idLobbyToSessionCBLocal;
	friend class idSessionLocalCallbacks;
	friend class idNetSessionPort;
	friend class lobbyAddress_t;

protected:
	//=====================================================================================================
	//	Mixed Common/Platform enums/structs
	//=====================================================================================================

	// Overall state of the session
	enum state_t
	{
		STATE_IDLE,									// We are at the main menu
		STATE_PARTY_LOBBY_HOST,						// We are in the party lobby menu as host
		STATE_PARTY_LOBBY_PEER,						// We are in the party lobby menu as a peer
		STATE_GAME_LOBBY_HOST,						// We are in the game lobby as a host
		STATE_GAME_LOBBY_PEER,						// We are in the game lobby as a peer
		STATE_GAME_STATE_LOBBY_HOST,				// We are in the game state lobby as a host
		STATE_GAME_STATE_LOBBY_PEER,				// We are in the game state lobby as a peer
		STATE_CREATE_AND_MOVE_TO_PARTY_LOBBY,		// We are creating a party lobby, and will move to that state when done
		STATE_CREATE_AND_MOVE_TO_GAME_LOBBY,		// We are creating a game lobby, and will move to that state when done
		STATE_CREATE_AND_MOVE_TO_GAME_STATE_LOBBY,	// We are creating a game state lobby, and will move to that state when done
		STATE_FIND_OR_CREATE_MATCH,
		STATE_CONNECT_AND_MOVE_TO_PARTY,
		STATE_CONNECT_AND_MOVE_TO_GAME,
		STATE_CONNECT_AND_MOVE_TO_GAME_STATE,
		STATE_BUSY,									// Doing something internally like a QoS/bandwidth challenge

		// These are last, so >= STATE_LOADING tests work
		STATE_LOADING,								// We are loading the map, preparing to go into a match
		STATE_INGAME,								// We are currently in a match
		NUM_STATES
	};

	enum connectType_t
	{
		CONNECT_NONE				= 0,
		CONNECT_DIRECT				= 1,
		CONNECT_FIND_OR_CREATE		= 2,
	};

	enum pendingInviteMode_t
	{
		PENDING_INVITE_NONE			= 0,		// No invite waiting
		PENDING_INVITE_WAITING		= 1,		// Invite is waiting
		PENDING_SELF_INVITE_WAITING	= 2,		// We invited ourselves to a match
	};

public:
	idSessionLocal();
	~idSessionLocal() override;

	void					InitBaseState();

	bool			IsPlatformPartyInLobby() override;



	// Lobby management
	void			CreatePartyLobby( const idMatchParameters& parms_ ) override;
	void			FindOrCreateMatch( const idMatchParameters& parms ) override;
	void			CreateMatch( const idMatchParameters& parms_ ) override;
	void			CreateGameStateLobby( const idMatchParameters& parms_ ) override;

	void			UpdatePartyParms( const idMatchParameters& parms_ ) override;
	void			UpdateMatchParms( const idMatchParameters& parms_ ) override;
	void			StartMatch() override;
	void			SetSessionOption( sessionOption_t option ) override
	{
		sessionOptions |= option;
	}
	void			ClearSessionOption( sessionOption_t option ) override
	{
		sessionOptions &= ~option;
	}
	sessionState_t	GetBackState() override;
	void			Cancel() override;
	void			FinishDisconnect() override;
	bool			ShouldShowMigratingDialog() const;	// Note this is not in sys_session.h
	bool			IsCurrentLobbyMigrating() const override;
	bool			IsLosingConnectionToHost() const override;

	// Migration
	bool			WasMigrationGame() const override;
	bool			ShouldRelaunchMigrationGame() const override;
	bool			GetMigrationGameData( idBitMsg& msg, bool reading ) override;
	bool			GetMigrationGameDataUser( lobbyUserID_t lobbyUserID, idBitMsg& msg, bool reading ) override;

	bool			WasGameLobbyCoalesced() const override
	{
		return gameLobbyWasCoalesced;
	}

	// Misc
	int				GetLoadingID() override
	{
		return loadingID;
	}
	bool			IsAboutToLoad() const override
	{
		return GetGameLobby().IsLobbyActive() && GetGameLobby().startLoadingFromHost;
	}
	bool			GetMatchParamUpdate( int& peer, int& msg ) override;
	int				GetInputRouting( int inputRouting[ MAX_INPUT_DEVICES ] ) override;
	void			EndMatch( bool premature = false ) override;			// Meant for host to end match gracefully, go back to lobby, tally scores, etc
	void			MatchFinished() override;							// this is for when the game is over before we go back to lobby. Need this incase the host leaves during this time
	void			QuitMatch() override;		// Meant for host or peer to quit the match before it ends, will instigate host migration, etc
	void			QuitMatchToTitle() override; // Will forcefully quit the match and return to the title screen.
	void			LoadingFinished() override;
	void			Pump() override;
	void			ProcessSnapAckQueue() override;

	sessionState_t	GetState() const override;
	const char* 	GetStateString() const override;

	void			SendUsercmds( idBitMsg& msg ) override;
	void			SendSnapshot( idSnapShot& ss ) override;
	const char* 	GetPeerName( int peerNum );

	const char* 	GetLocalUserName( int i ) const override
	{
		return signInManager->GetLocalUserByIndex( i )->GetGamerTag();
	}
	void			UpdateSignInManager() override;
	idPlayerProfile* GetProfileFromMasterLocalUser() override;

	void			PrePickNewHost( idLobby& lobby, bool forceMe, bool inviteOldHost );
	bool			PreMigrateInvite( idLobby& lobby );

	//=====================================================================================================
	// Title Storage Vars
	//=====================================================================================================
	float			GetTitleStorageFloat( const char* name, float defaultFloat ) const override
	{
		return titleStorageVars.GetFloat( name, defaultFloat );
	}
	int				GetTitleStorageInt( const char* name, int defaultInt ) const override
	{
		return titleStorageVars.GetInt( name, defaultInt );
	}
	bool			GetTitleStorageBool( const char* name, bool defaultBool ) const override
	{
		return titleStorageVars.GetBool( name, defaultBool );
	}
	const char* 	GetTitleStorageString( const char* name, const char* defaultString ) const override
	{
		return titleStorageVars.GetString( name, defaultString );
	}

	bool			GetTitleStorageFloat( const char* name, float defaultFloat, float& out ) const override
	{
		return titleStorageVars.GetFloat( name, defaultFloat, out );
	}
	bool			GetTitleStorageInt( const char* name, int defaultInt, int& out ) const override
	{
		return titleStorageVars.GetInt( name, defaultInt, out );
	}
	bool			GetTitleStorageBool( const char* name, bool defaultBool, bool& out ) const override
	{
		return titleStorageVars.GetBool( name, defaultBool, out );
	}
	bool			GetTitleStorageString( const char* name, const char* defaultString, const char** out ) const override
	{
		return titleStorageVars.GetString( name, defaultString, out );
	}

	bool			IsTitleStorageLoaded() override
	{
		return titleStorageLoaded;
	}

	//=====================================================================================================
	//	Voice chat
	//=====================================================================================================
	voiceState_t		GetLobbyUserVoiceState( lobbyUserID_t lobbyUserID ) override;
	voiceStateDisplay_t GetDisplayStateFromVoiceState( voiceState_t voiceState ) const override;
	void				ToggleLobbyUserVoiceMute( lobbyUserID_t lobbyUserID ) override;
	void				SetActiveChatGroup( int groupIndex ) override;
	void				UpdateMasterUserHeadsetState();

	//=====================================================================================================
	//	Bandwidth / QoS checking
	//=====================================================================================================
	bool			StartOrContinueBandwidthChallenge( bool forceStart ) override;
	void			DebugSetPeerSnaprate( int peerIndex, int snapRateMS ) override;
	float			GetIncomingByteRate() override;

	//=====================================================================================================
	// Invites
	//=====================================================================================================
	void			HandleBootableInvite( int64 lobbyId = 0 ) override;

	bool					GetFlushedStats()
	{
		return flushedStats;
	}
	void					SetFlushedStats( bool _flushedStats )
	{
		flushedStats = _flushedStats;
	}

	//=====================================================================================================
	// Notifications
	//=====================================================================================================
	// This is called when a LocalUser is signed in/out
	void			OnLocalUserSignin( idLocalUser* user ) override;
	void			OnLocalUserSignout( idLocalUser* user ) override;

	// This is called when the master LocalUser is signed in/out, these are called after OnLocalUserSignin/out()
	void			OnMasterLocalUserSignout() override;
	void			OnMasterLocalUserSignin() override;

	// After a local user has signed in and their profile has loaded
	void			OnLocalUserProfileLoaded( idLocalUser* user ) override;

	//=====================================================================================================
	//	Platform specific (different platforms implement these differently)
	//=====================================================================================================

	void			Initialize() override;
	void			Shutdown() override;

	void			InitializeSoundRelatedSystems() override;
	void			ShutdownSoundRelatedSystems() override;


	void			InviteFriends() override;
	void			InviteParty() override;
	void			ShowPartySessions() override;

	bool			ProcessInputEvent( const sysEvent_t* ev ) override;

	// Play with Friends server listing
	int				NumServers() const override;
	void			ListServers( const idCallback& callback ) override;
	void			ListServersCommon();
	void			CancelListServers() override;
	void			ConnectToServer( int i ) override;
	const serverInfo_t* ServerInfo( int i ) const override;
	const idList< idStr >* ServerPlayerList( int i ) override;
	void			ShowServerGamerCardUI( int i ) override;
	void					ShowOnlineSignin() override;

	void 			HandleServerQueryRequest( lobbyAddress_t& remoteAddr, idBitMsg& msg, int msgType );
	void 			HandleServerQueryAck( lobbyAddress_t& remoteAddr, idBitMsg& msg );

	// System UI
	bool			IsSystemUIShowing() const override;
	void			SetSystemUIShowing( bool show ) override;

	void			ShowLobbyUserGamerCardUI( lobbyUserID_t lobbyUserID ) override;

	// Leaderboards
	const leaderboardDefinition_t* LeaderboardFindDef( int id ) override;
	leaderboardDefinition_t* LeaderboardCreateDef( int id_, int numColumns_, const columnDef_t* columnDefs_, int rankOrder_, bool supportsAttachments_, bool checkAgainstCurrent_ ) override;
	void			LeaderboardDestroyDefs() override;
	void			LeaderboardUpload( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t* leaderboard, const column_t* stats, const idFile_Memory* attachment = NULL ) override;
	void			LeaderboardDownload( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int startingRank, int numRows, const idLeaderboardCallback& callback ) override;
	void			LeaderboardDownloadAttachment( int sessionUserIndex, const leaderboardDefinition_t* leaderboard, int64 attachmentID ) override;

	// Scoring (currently just for TrueSkill)

	void			LeaderboardFlush() override;

	//=====================================================================================================i'
	//	Savegames
	//=====================================================================================================
	saveGameHandle_t	SaveGameSync( const char* name, const saveFileEntryList_t& files, const idSaveGameDetails& description ) override;
	saveGameHandle_t	SaveGameAsync( const char* name, const saveFileEntryList_t& files, const idSaveGameDetails& description ) override;
	saveGameHandle_t	LoadGameSync( const char* name, saveFileEntryList_t& files ) override;
	saveGameHandle_t	EnumerateSaveGamesSync() override;
	saveGameHandle_t	EnumerateSaveGamesAsync() override;
	saveGameHandle_t	DeleteSaveGameSync( const char* name ) override;
	saveGameHandle_t	DeleteSaveGameAsync( const char* name ) override;

	bool							IsSaveGameCompletedFromHandle( const saveGameHandle_t& handle ) const override
	{
		return saveGameManager->IsSaveGameCompletedFromHandle( handle );
	}
	void							CancelSaveGameWithHandle( const saveGameHandle_t& handle ) override
	{
		GetSaveGameManager().CancelWithHandle( handle );
	}
	const saveGameDetailsList_t& 	GetEnumeratedSavegames() const override
	{
		return saveGameManager->GetEnumeratedSavegames();
	}
	bool							IsEnumerating() const override;
	saveGameHandle_t				GetEnumerationHandle() const override;
	void							SetCurrentSaveSlot( const char* slotName ) override
	{
		currentSaveSlot = slotName;
	}
	const char* 					GetCurrentSaveSlot() const override
	{
		return currentSaveSlot.c_str();
	}

	// Notifications
	void					OnSaveCompleted( idSaveLoadParms* parms );
	void					OnLoadCompleted( idSaveLoadParms* parms );
	void					OnDeleteCompleted( idSaveLoadParms* parms );
	void					OnEnumerationCompleted( idSaveLoadParms* parms );

	// Error checking
	bool					LoadGameCheckDescriptionFile( idSaveLoadParms& parms );


	void					DropClient( int peerNum, int session );

protected:

	float					GetUpstreamDropRate()
	{
		return upstreamDropRate;
	}
	float					GetUpstreamQueueRate()
	{
		return upstreamQueueRate;
	}
	int						GetQueuedBytes()
	{
		return queuedBytes;
	}

	//=====================================================================================================
	// Common functions (sys_session_local.cpp)
	//=====================================================================================================
	void					HandleLobbyControllerState( int lobbyType );
	void			UpdatePendingInvite();
	bool					HandleState();

	// The party and game lobby are the two platform lobbies that notify the backends (Steam/PSN/LIVE of changes)
	idLobby& 				GetPartyLobby()
	{
		return partyLobby;
	}
	const idLobby& 			GetPartyLobby() const
	{
		return partyLobby;
	}
	idLobby& 				GetGameLobby()
	{
		return gameLobby;
	}
	const idLobby& 			GetGameLobby() const
	{
		return gameLobby;
	}

	// Game state lobby is the lobby used while in-game.  It is so the dedicated server can host this lobby
	// and have all platform clients join. It does NOT notify the backends of changes, it's purely for the dedicated
	// server to be able to host the in-game lobby.
	// Generally, you would call GetActingGameStateLobby.  If we are not using game state lobby, GetActingGameStateLobby will return GetGameLobby insread.
	idLobby& 				GetGameStateLobby()
	{
		return gameStateLobby;
	}
	const idLobby& 			GetGameStateLobby() const
	{
		return gameStateLobby;
	}

	idLobby& 				GetActingGameStateLobby();
	const idLobby& 			GetActingGameStateLobby() const;

	// GetActivePlatformLobby will return either the game or party lobby, it won't return the game state lobby
	// This function is generally used for menus, in-game code should refer to GetActingGameStateLobby
	idLobby* 				GetActivePlatformLobby();
	const idLobby* 			GetActivePlatformLobby() const;

	idLobby* 				GetLobbyFromType( idLobby::lobbyType_t lobbyType );

	idLobbyBase& 	GetPartyLobbyBase() override
	{
		return partyLobby;
	}
	idLobbyBase& 	GetGameLobbyBase() override
	{
		return gameLobby;
	}
	idLobbyBase& 	GetActingGameStateLobbyBase() override
	{
		return GetActingGameStateLobby();
	}

	idLobbyBase& 	GetActivePlatformLobbyBase() override;
	idLobbyBase& 	GetLobbyFromLobbyUserID( lobbyUserID_t lobbyUserID ) override;

	void					SetState( state_t newState );
	bool					HandlePackets();

	void					HandleVoiceRestrictionDialog();
	void					SetDroppedByHost( bool dropped )
	{
		droppedByHost = dropped;
	}
	bool					GetDroppedByHost()
	{
		return droppedByHost;
	}

public:
	int						storedPeer;
	int						storedMsgType;

protected:
	static const char* 		stateToString[ NUM_STATES ];

	state_t					localState;
	uint32					sessionOptions;

	connectType_t			connectType;
	int						connectTime;

	idLobby					partyLobby;
	idLobby					gameLobby;
	idLobby					gameStateLobby;
	idLobbyStub				stubLobby;				// We use this when we request the active lobby when we are not in a lobby (i.e at press start)

	int						currentID;				// The host used this to send out a unique id to all users so we can identify them

	class idVoiceChatMgr* 	voiceChat;
	int						lastVoiceSendtime;
	bool					hasShownVoiceRestrictionDialog;

	pendingInviteMode_t		pendingInviteMode;
	int						pendingInviteDevice;
	lobbyConnectInfo_t		pendingInviteConnectInfo;

	bool					isSysUIShowing;

	idDict					titleStorageVars;
	bool					titleStorageLoaded;

	int						showMigratingInfoStartTime;

	int						nextGameCoalesceTime;
	bool					gameLobbyWasCoalesced;
	int						numFullSnapsReceived;

	bool					flushedStats;

	int						loadingID;


	idSaveGameProcessorSaveFiles* processorSaveFiles;
	idSaveGameProcessorLoadFiles* processorLoadFiles;
	idSaveGameProcessorDelete* processorDelete;
	idSaveGameProcessorEnumerateGames* processorEnumerate;

	idStr							currentSaveSlot;
	saveGameHandle_t				enumerationHandle;

	//------------------------
	// State functions
	//------------------------
	bool	State_Party_Lobby_Host();
	bool	State_Party_Lobby_Peer();
	bool	State_Game_Lobby_Host();
	bool	State_Game_Lobby_Peer();
	bool	State_Game_State_Lobby_Host();
	bool	State_Game_State_Lobby_Peer();
	bool	State_Loading();
	bool	State_InGame();
	bool	State_Find_Or_Create_Match();
	bool	State_Create_And_Move_To_Party_Lobby();
	bool	State_Create_And_Move_To_Game_Lobby();
	bool	State_Create_And_Move_To_Game_State_Lobby();

	bool	State_Connect_And_Move_To_Party();
	bool	State_Connect_And_Move_To_Game();
	bool	State_Connect_And_Move_To_Game_State();
	bool	State_Finalize_Connect();
	bool	State_Busy();

	class idQueuePacket
	{
	public:
		byte						data[ idPacketProcessor::MAX_FINAL_PACKET_SIZE ];
		lobbyAddress_t				address;
		int							size;
		int							time;
		bool						dedicated;
		idQueueNode<idQueuePacket>	queueNode;
	};

	idBlockAlloc< idQueuePacket, 64, TAG_NETWORKING >	packetAllocator;
	idQueue< idQueuePacket, &idQueuePacket::queueNode >	sendQueue;
	idQueue< idQueuePacket, &idQueuePacket::queueNode >	recvQueue;

	float												upstreamDropRate;		// instant rate in B/s at which we are dropping packets due to simulated upstream saturation
	int													upstreamDropRateTime;

	float												upstreamQueueRate;		// instant rate in B/s at which queued packets are coming out after local buffering due to upstream saturation
	int													upstreamQueueRateTime;

	int													queuedBytes;

	int													waitingOnGameStateMembersToLeaveTime;
	int													waitingOnGameStateMembersToJoinTime;

	void	TickSendQueue();

	void	QueuePacket( idQueue< idQueuePacket, &idQueuePacket::queueNode >& queue, int time, const lobbyAddress_t& to, const void* data, int size, bool dedicated );
	bool	ReadRawPacketFromQueue( int time, lobbyAddress_t& from, void* data, int& size, bool& outDedicated, int maxSize );

	void	SendRawPacket( const lobbyAddress_t& to, const void* data, int size, bool dedicated );
	bool	ReadRawPacket( lobbyAddress_t& from, void* data, int& size, bool& outDedicated, int maxSize );

	void	ConnectAndMoveToLobby( idLobby& lobby, const lobbyConnectInfo_t& connectInfo, bool fromInvite );
	void	GoodbyeFromHost( idLobby& lobby, int peerNum, const lobbyAddress_t& remoteAddress, int msgType );

	void	WriteLeaderboardToMsg( idBitMsg& msg, const leaderboardDefinition_t* leaderboard, const column_t* stats );
	void	SendLeaderboardStatsToPlayer( lobbyUserID_t lobbyUserID, const leaderboardDefinition_t* leaderboard, const column_t* stats );
	void	RecvLeaderboardStatsForPlayer( idBitMsg& msg );

	const leaderboardDefinition_t* ReadLeaderboardFromMsg( idBitMsg& msg, column_t* stats );
	bool	RequirePersistentMaster();

	idNetSessionPort& 	GetPort( bool dedicated = false );
	idLobbyBackend* 	CreateLobbyBackend( const idMatchParameters& p, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType );
	idLobbyBackend* 	FindLobbyBackend( const idMatchParameters& p, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType );
	idLobbyBackend* 	JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo , idLobbyBackend::lobbyBackendType_t lobbyType );
	void				DestroyLobbyBackend( idLobbyBackend* lobby );
	void				PumpLobbies();
	bool				GetLobbyAddressFromNetAddress( const netadr_t& netAddr, lobbyAddress_t& outAddr ) const;
	bool				GetNetAddressFromLobbyAddress( const lobbyAddress_t& lobbyAddress, netadr_t& outNetAddr ) const;

	void 	HandleDedicatedServerQueryRequest( lobbyAddress_t& remoteAddr, idBitMsg& msg, int msgType );
	void 	HandleDedicatedServerQueryAck( lobbyAddress_t& remoteAddr, idBitMsg& msg );


	void	ClearMigrationState();
	// this is called when the mathc is over and returning to lobby
	void	EndMatchInternal( bool premature = false );
	// this is called when the game finished and we are in the end match recap
	void	MatchFinishedInternal();
	void	EndMatchForMigration();

	// Voice chat
	void	SendVoiceAudio();
	void	HandleOobVoiceAudio( const lobbyAddress_t& from, const idBitMsg& msg );
	void	SetVoiceGroupsToTeams();
	void	ClearVoiceGroups();

	// All the new functions going here for now until it can all be cleaned up
	void	StartSessions();
	void	EndSessions();
	void	SetLobbiesAreJoinable( bool joinable );
	void	MoveToMainMenu();				// End all session (async), and return to IDLE state
	bool	WaitOnLobbyCreate( idLobby& lobby );
	bool	DetectDisconnectFromService( bool cancelAndShowMsg );
	void	HandleConnectionFailed( idLobby& lobby, bool wasFull );
	void	ConnectToNextSearchResultFailed( idLobby& lobby );
	bool	HandleConnectAndMoveToLobby( idLobby& lobby );

	void	VerifySnapshotInitialState();

	void	ComputeNextGameCoalesceTime();

	void	StartLoading();

	bool	ShouldHavePartyLobby();
	void	ValidateLobbies();
	void	ValidateLobby( idLobby& lobby );



	idSessionCallbacks*	 sessionCallbacks;

	int		offlineTransitionTimerStart;

	bool	droppedByHost;



public:
	//------------------------
	// Service objects
	//------------------------
	idSignInManager&		GetSignInManager() override
	{
		return *signInManager;
	}
	idSaveGameManager&		GetSaveGameManager() override
	{
		return *saveGameManager;
	}
	idAchievementSystem&	GetAchievementSystem() override
	{
		return *achievementSystem;
	}
	bool					HasSignInManager() const override
	{
		return ( signInManager != NULL );
	}
	bool					HasAchievementSystem() const override
	{
		return ( achievementSystem != NULL );
	}

	void					Connect_f( const idCmdArgs& args );

private:
	idSignInManager*			signInManager;
	idSaveGameManager*			saveGameManager;
	idAchievementSystem*		achievementSystem;
	idDedicatedServerSearch*	dedicatedServerSearch;

	// Direct (LAN / dedicated server) lobby backends
	idArray< idLobbyBackend*, 3 >	lobbyBackends;
	idNetSessionPort				port;
	bool							canJoinLocalHost;

	void					EnsurePort();
	void					DestroyServices();
	idLobbyBackend*			CreateLobbyInternal( idLobbyBackend::lobbyBackendType_t lobbyType );
};

/*
========================
idSessionLocalCallbacks
	The more the idLobby class needs to call back into this class, the more likely we're doing something wrong, and there is a better way.
========================
*/
class idSessionLocalCallbacks : public idSessionCallbacks
{
public:
	idSessionLocalCallbacks( idSessionLocal* sessionLocal_ )
	{
		sessionLocal = sessionLocal_;
	}

	virtual							~idSessionLocalCallbacks() {}

	virtual idLobby& 				GetPartyLobby()
	{
		return sessionLocal->GetPartyLobby();
	}
	virtual idLobby& 				GetGameLobby()
	{
		return sessionLocal->GetGameLobby();
	}
	virtual idLobby& 				GetActingGameStateLobby()
	{
		return sessionLocal->GetActingGameStateLobby();
	}
	virtual idLobby* 				GetLobbyFromType( idLobby::lobbyType_t lobbyType )
	{
		return sessionLocal->GetLobbyFromType( lobbyType );
	}

	virtual int						GetUniquePlayerId() const
	{
		return sessionLocal->currentID++;
	}
	virtual idSignInManager&		GetSignInManager()
	{
		return *sessionLocal->signInManager;
	}
	virtual	void					SendRawPacket( const lobbyAddress_t& to, const void* data, int size, bool useDirectPort )
	{
		sessionLocal->SendRawPacket( to, data, size, useDirectPort );
	}

	virtual bool					BecomingHost( idLobby& lobby );
	virtual void					BecameHost( idLobby& lobby );
	virtual bool					BecomingPeer( idLobby& lobby );
	virtual void					BecamePeer( idLobby& lobby );

	virtual void					FailedGameMigration( idLobby& lobby );
	virtual void					MigrationEnded( idLobby& lobby );

	virtual void					GoodbyeFromHost( idLobby& lobby, int peerNum, const lobbyAddress_t& remoteAddress, int msgType );
	virtual	uint32					GetSessionOptions()
	{
		return sessionLocal->sessionOptions;
	}

	virtual bool					AnyPeerHasAddress( const lobbyAddress_t& remoteAddress ) const;

	virtual idSession::sessionState_t GetState() const
	{
		return sessionLocal->GetState();
	}

	virtual void					ClearMigrationState()
	{
		GetPartyLobby().ResetAllMigrationState();
		GetGameLobby().ResetAllMigrationState();
	}

	virtual void					EndMatchInternal( bool premature = false )
	{
		sessionLocal->EndMatchInternal( premature );
	}

	virtual void					RecvLeaderboardStats( idBitMsg& msg );

	virtual void					ReceivedFullSnap();

	virtual void					LeaveGameLobby();

	virtual void					PrePickNewHost( idLobby& lobby, bool forceMe, bool inviteOldHost );
	virtual bool					PreMigrateInvite( idLobby& lobby );

	virtual void					HandleOobVoiceAudio( const lobbyAddress_t& from, const idBitMsg& msg )
	{
		sessionLocal->HandleOobVoiceAudio( from, msg );
	}

	virtual void					ConnectAndMoveToLobby( idLobby::lobbyType_t destLobbyType, const lobbyConnectInfo_t& connectInfo, bool waitForPartyOk );

	virtual idVoiceChatMgr* 		GetVoiceChat()
	{
		return sessionLocal->voiceChat;
	}

	virtual void					HandleServerQueryRequest( lobbyAddress_t& remoteAddr, idBitMsg& msg, int msgType );
	virtual void 					HandleServerQueryAck( lobbyAddress_t& remoteAddr, idBitMsg& msg );

	virtual void 					HandlePeerMatchParamUpdate( int peer, int msg );

	virtual idLobbyBackend* 		CreateLobbyBackend( const idMatchParameters& p, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual idLobbyBackend* 		FindLobbyBackend( const idMatchParameters& p, int numPartyUsers, float skillLevel, idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual idLobbyBackend* 		JoinFromConnectInfo( const lobbyConnectInfo_t& connectInfo , idLobbyBackend::lobbyBackendType_t lobbyType );
	virtual void					DestroyLobbyBackend( idLobbyBackend* lobby );

	idSessionLocal* sessionLocal;
};

