/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#ifndef __GAME_H__
#define __GAME_H__

/*
===============================================================================

	Public game interface with methods to run the game.

===============================================================================
*/

// default scripts
#define SCRIPT_DEFAULTDEFS			"script/doom_defs.script"
#define SCRIPT_DEFAULT				"script/doom_main.script"
#define SCRIPT_DEFAULTFUNC			"doom_main"

struct gameReturn_t
{

	gameReturn_t() :
		sessionCommand( "" ),       // SRS - Explicitly init sessionCommand otherwise can be optimized out and skipped with gcc or Apple clang
		syncNextGameFrame( false ),
		vibrationLow( 0 ),
		vibrationHigh( 0 )
	{

	}

	char		sessionCommand[MAX_STRING_CHARS];	// "map", "disconnect", "victory", etc
	bool		syncNextGameFrame;					// used when cinematics are skipped to prevent session from simulating several game frames to
	// keep the game time in sync with real time
	int			vibrationLow;
	int			vibrationHigh;
};

#define TIME_GROUP1		0
#define TIME_GROUP2		1

class idGame
{
public:
	virtual						~idGame() {}

	// Initialize the game for the first time.
	virtual void				Init() = 0;

	// Shut down the entire game.
	virtual void				Shutdown() = 0;

	// Sets the serverinfo at map loads and when it changes.
	virtual void				SetServerInfo( const idDict& serverInfo ) = 0;

	// Gets the serverinfo, common calls this before saving the game
	virtual const idDict& 		GetServerInfo() = 0;

	// Interpolated server time
	virtual void				SetServerGameTimeMs( const int time ) = 0;

	// Interpolated server time
	virtual int					GetServerGameTimeMs() const = 0;

	virtual int					GetSSEndTime() const  = 0;
	virtual int					GetSSStartTime() const = 0;

	// common calls this before moving the single player game to a new level.
	virtual const idDict& 		GetPersistentPlayerInfo( int clientNum ) = 0;

	// common calls this right before a new level is loaded.
	virtual void				SetPersistentPlayerInfo( int clientNum, const idDict& playerInfo ) = 0;

	// Loads a map and spawns all the entities.
	virtual void				InitFromNewMap( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, int gameMode, int randseed ) = 0;

	// Loads a map from a savegame file.
	virtual bool				InitFromSaveGame( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, idFile* saveGameFile, idFile* stringTableFile, int saveGameVersion ) = 0;

	// Saves the current game state, common may have written some data to the file already.
	virtual void				SaveGame( idFile* saveGameFile, idFile* stringTableFile ) = 0;

	// Pulls the current player location from the game information
	virtual void				GetSaveGameDetails( idSaveGameDetails& gameDetails ) = 0;

	// Shut down the current map.
	virtual void				MapShutdown() = 0;

	// Caches media referenced from in key/value pairs in the given dictionary.
	virtual void				CacheDictionaryMedia( const idDict* dict ) = 0;

	virtual void				Preload( const idPreloadManifest& manifest ) = 0;

	// Runs a game frame, may return a session command for level changing, etc
	virtual void				RunFrame( idUserCmdMgr& cmdMgr, gameReturn_t& gameReturn ) = 0;

	// Makes rendering and sound system calls to display for a given clientNum.
	virtual bool				Draw( int clientNum ) = 0;

	virtual bool				HandlePlayerGuiEvent( const sysEvent_t* ev ) = 0;

	// Writes a snapshot of the server game state.
	virtual void				ServerWriteSnapshot( idSnapShot& ss ) = 0;

	// Processes a reliable message
	virtual void				ProcessReliableMessage( int clientNum, int type, const idBitMsg& msg ) = 0;

	virtual void				SetInterpolation( const float fraction, const int serverGameMS, const int ssStartTime, const int ssEndTime ) = 0;

	// Reads a snapshot and updates the client game state.
	virtual void				ClientReadSnapshot( const idSnapShot& ss ) = 0;

	// Runs prediction on entities at the client.
	virtual void				ClientRunFrame( idUserCmdMgr& cmdMgr, bool lastPredictFrame, gameReturn_t& ret ) = 0;

	// Used to manage divergent time-lines
	virtual int					GetTimeGroupTime( int timeGroup ) = 0;

	// Returns a list of available multiplayer game modes
	virtual int					GetMPGameModes( const char** * gameModes, const char** * gameModesDisplay ) = 0;

	// Returns a summary of stats for a given client
	virtual void				GetClientStats( int clientNum, char* data, const int len ) = 0;

	virtual bool				IsInGame() const = 0;

	// Get the player entity number for a network peer.
	virtual int					MapPeerToClient( int peer ) const = 0;

	// Get the player entity number of the local player.
	virtual int					GetLocalClientNum() const = 0;

	// compute an angle offset to be applied to the given client's aim
	virtual void				GetAimAssistAngles( idAngles& angles ) = 0;
	virtual float				GetAimAssistSensitivity() = 0;

	// Syncs the player entities with the lobby users, so that the correct player models and names show up in the lobby and pause menu.
	virtual void				SyncPlayersWithLobbyUsers() = 0;

	// Get the current tonemap settings for the player's view, used for syncing with the backend.
	virtual bool				GetActiveTonemapState( int& preset, float& exposure, float& saturation, float& contrast, float& hdrKey ) = 0;

	// Release the mouse when the PDA/Chat area is open
	virtual bool				IsPDAOpen() const = 0;
	virtual bool				IsPlayerChatting() const = 0;
	virtual bool				InhibitControls() = 0;

	// Skip Cinematic process
	virtual bool				SkipCinematicScene() = 0;
	virtual bool				CheckInCinematic() = 0;

	// Demo helper functions
	virtual void				StartDemoPlayback( idRenderWorld* renderworld ) = 0;
	virtual bool				ProcessDemoCommand( idDemoFile* readDemo ) = 0;
};

extern idGame* 					game;

/*
================================================================================================

	Main Menu

================================================================================================
*/

class idGameMainMenu
{
public:
	virtual						~idGameMainMenu() {}

	// Initiallize the menu system.
	virtual void				Initialize() = 0;
	virtual void				Shutdown() = 0;

	// MAIN MENU FUNCTIONS
	virtual void				Init( const char* filename, idSoundWorld* sw ) = 0;
	virtual void				InitMenu() = 0;
	virtual bool				IsLoadingActive() const = 0;
	virtual void				LoadingGui( const char* mapName, bool& hellMap ) = 0;
	virtual void				RenderLoadingShell() = 0;
	virtual void				Cleanup( bool onlyLoading = false ) = 0;
	virtual void				CreateMenu( bool inGame ) = 0;
	virtual void				ClosePause() = 0;
	virtual void				Show( bool show ) = 0;
	virtual bool				IsActive() const = 0;
	virtual bool				HandleGuiEvent( const sysEvent_t* sev ) = 0;
	virtual void				Render() = 0;
	virtual void				ResetMenu() = 0;
	virtual void				SyncWithSession() = 0;
	virtual void				UpdateSavedGames() = 0;
	virtual void				SetCanContinue( bool valid ) = 0;
	virtual void				UpdateClientCountdown( int countdown ) = 0;
	virtual void				UpdateLeaderboard( const idLeaderboardCallback* callback ) = 0;
	virtual void				SetGameComplete() = 0;
	virtual bool				IsShowingIntro() = 0;
	virtual bool				IsGameComplete() = 0;
};

extern idGameMainMenu* 		mainMenu;

/*
================================================================================================

	Dialogs

================================================================================================
*/

static const int	MAX_DIALOGS			= 4;		// maximum dialogs that can be open at one time
static const int	PC_KEYBOARD_WAIT	= 20000;

/*
================================================
Dialog box message types
================================================
*/
enum gameDialogMessages_t
{
	GDM_INVALID,
	GDM_SWAP_DISKS_TO1,
	GDM_SWAP_DISKS_TO2,
	GDM_SWAP_DISKS_TO3,
	GDM_NO_GAMER_PROFILE,
	GDM_PLAY_ONLINE_NO_PROFILE,
	GDM_LEADERBOARD_ONLINE_NO_PROFILE,
	GDM_NO_STORAGE_SELECTED,
	GDM_ONLINE_INCORRECT_PERMISSIONS,
	GDM_SP_QUIT_SAVE,
	GDM_SP_RESTART_SAVE,
	GDM_SP_SIGNIN_CHANGE,
	GDM_SERVER_NOT_AVAILABLE,
	GDM_CONNECTION_LOST_HOST,
	GDM_CONNECTION_LOST,
	GDM_OPPONENT_CONNECTION_LOST,
	GDM_HOST_CONNECTION_LOST,
	GDM_HOST_CONNECTION_LOST_STATS,
	GDM_FAILED_TO_LOAD_RANKINGS,
	GDM_HOST_QUIT,
	GDM_BECAME_HOST_PARTY,			// Became host of party
	GDM_NEW_HOST_PARTY,				// Someone else became host of party
	GDM_LOBBY_BECAME_HOST_GAME,		// In lobby, you became game host
	GDM_LOBBY_NEW_HOST_GAME,		// In lobby, new game host was chosen (not you)
	GDM_NEW_HOST_GAME,				// Host left/DC'd, someone else is new host, unranked game
	GDM_NEW_HOST_GAME_STATS_DROPPED,// Host left/DC'd, someone else is new host, ranked game so stats were dropped
	GDM_BECAME_HOST_GAME,				// Host left/DC'd, you became host, unranked game
	GDM_BECAME_HOST_GAME_STATS_DROPPED, // Host left/DC'd, you became host, ranked game so stats were dropped
	GDM_LOBBY_DISBANDED,
	GDM_LEAVE_WITH_PARTY,
	GDM_LEAVE_LOBBY_RET_MAIN,
	GDM_LEAVE_LOBBY_RET_NEW_PARTY,
	GDM_MIGRATING,
	GDM_OPPONENT_LEFT,
	GDM_NO_MATCHES_FOUND,
	GDM_INVALID_INVITE,
	GDM_KICKED,
	GDM_BANNED,
	GDM_SAVING,
	GDM_OVERWRITE_SAVE,
	GDM_LOAD_REQUEST,
	GDM_AUTOSAVE_DISABLED_STORAGE_REMOVED,
	GDM_STORAGE_INVALID,
	GDM_STORAGE_REMOVED,
	GDM_CONNECTING,
	GDM_REFRESHING,
	GDM_DELETE_SAVE,
	GDM_DELETING,
	GDM_BINDING_ALREDY_SET,
	GDM_CANNOT_BIND,
	GDM_OVERLAY_DISABLED,
	GDM_DIRECT_MAP_CHANGE,
	GDM_DELETE_AUTOSAVE,
	GDM_QUICK_SAVE,
	GDM_MULTI_RETRY,
	GDM_MULTI_SELF_DESTRUCT,
	GDM_MULTI_VDM_QUIT,
	GDM_MULTI_COOP_QUIT,
	GDM_LOADING_PROFILE,
	GDM_STORAGE_REQUIRED,
	GDM_INSUFFICENT_STORAGE_SPACE,
	GDM_PARTNER_LEFT,
	GDM_RESTORE_CORRUPT_SAVEGAME,
	GDM_UNRECOVERABLE_SAVEGAME,
	GDM_PROFILE_SAVE_ERROR,
	GDM_LOBBY_FULL,
	GDM_QUIT_GAME,
	GDM_CONNECTION_PROBLEMS,
	GDM_VOICE_RESTRICTED,
	GDM_LOAD_DAMAGED_FILE,
	GDM_MUST_SIGNIN,
	GDM_CONNECTION_LOST_NO_LEADERBOARD,
	GDM_SP_SIGNIN_CHANGE_POST,
	GDM_MIGRATING_WAITING,
	GDM_MIGRATING_RELAUNCHING,
	GDM_MIGRATING_FAILED_CONNECTION,
	GDM_MIGRATING_FAILED_CONNECTION_STATS,
	GDM_MIGRATING_FAILED_DISBANDED,
	GDM_MIGRATING_FAILED_DISBANDED_STATS,
	GDM_MIGRATING_FAILED_PARTNER_LEFT,
	GDM_HOST_RETURNED_TO_LOBBY,
	GDM_HOST_RETURNED_TO_LOBBY_STATS_DROPPED,
	GDM_FAILED_JOIN_LOCAL_SESSION,
	GDM_DELETE_CORRUPT_SAVEGAME,
	GDM_LEAVE_INCOMPLETE_INSTANCE,
	GDM_UNBIND_CONFIRM,
	GDM_BINDINGS_RESTORE,
	GDM_NEW_HOST,
	GDM_CONFIRM_VIDEO_CHANGES,
	GDM_UNABLE_TO_USE_SELECTED_STORAGE_DEVICE,
	GDM_ERROR_LOADING_SAVEGAME,
	GDM_ERROR_SAVING_SAVEGAME,
	GDM_DISCARD_CHANGES,
	GDM_LEAVE_LOBBY,
	GDM_LEAVE_LOBBY_AND_TEAM,
	GDM_CONTROLLER_DISCONNECTED_0,
	GDM_CONTROLLER_DISCONNECTED_1,
	GDM_CONTROLLER_DISCONNECTED_2,
	GDM_CONTROLLER_DISCONNECTED_3,
	GDM_CONTROLLER_DISCONNECTED_4,
	GDM_CONTROLLER_DISCONNECTED_5,
	GDM_CONTROLLER_DISCONNECTED_6,
	GDM_DLC_ERROR_REMOVED,
	GDM_DLC_ERROR_CORRUPT,
	GDM_DLC_ERROR_MISSING,
	GDM_DLC_ERROR_MISSING_GENERIC,
	GDM_DISC_SWAP,
	GDM_NEEDS_INSTALL,
	GDM_NO_SAVEGAMES_AVAILABLE,
	GDM_ERROR_JOIN_TWO_PROFILES_ONE_BOX,
	GDM_WARNING_PLAYING_COOP_SOLO,
	GDM_MULTI_COOP_QUIT_LOSE_LEADERBOARDS,
	GDM_CORRUPT_CONTINUE,
	GDM_MULTI_VDM_QUIT_LOSE_LEADERBOARDS,
	GDM_WARNING_PLAYING_VDM_SOLO,
	GDM_NO_GUEST_SUPPORT,
	GDM_DISC_SWAP_CONFIRMATION,
	GDM_ERROR_LOADING_PROFILE,
	GDM_CANNOT_INVITE_LOBBY_FULL,
	GDM_WARNING_FOR_NEW_DEVICE_ABOUT_TO_LOSE_PROGRESS,
	GDM_DISCONNECTED,
	GDM_INCOMPATIBLE_NEWER_SAVE,
	GDM_ACHIEVEMENTS_DISABLED_DUE_TO_CHEATING,
	GDM_INCOMPATIBLE_POINTER_SIZE,
	GDM_TEXTUREDETAIL_RESTARTREQUIRED,
	GDM_TEXTUREDETAIL_INSUFFICIENT_CPU,
	GDM_CHECKPOINT_SAVE,
	GDM_CALCULATING_BENCHMARK,
	GDM_DISPLAY_BENCHMARK,
	GDM_DISPLAY_CHANGE_FAILED,
	GDM_GPU_TRANSCODE_FAILED,
	GDM_OUT_OF_MEMORY,
	GDM_CORRUPT_PROFILE,
	GDM_PROFILE_TOO_OUT_OF_DATE_DEVELOPMENT_ONLY,
	GDM_SP_LOAD_SAVE,
	GDM_INSTALLING_TROPHIES,
	GDM_XBOX_DEPLOYMENT_TYPE_FAIL,
	GDM_SAVEGAME_WRONG_LANGUAGE,
	GDM_GAME_RESTART_REQUIRED,
	GDM_GAME_ERROR,
	GDM_MAX
};

/*
================================================
Dialog box types
================================================
*/
enum dialogType_t
{
	DIALOG_INVALID = -1,
	DIALOG_ACCEPT,
	DIALOG_CONTINUE,
	DIALOG_ACCEPT_CANCEL,
	DIALOG_YES_NO,
	DIALOG_CANCEL,
	DIALOG_WAIT,
	DIALOG_WAIT_BLACKOUT,
	DIALOG_WAIT_CANCEL,
	DIALOG_DYNAMIC,
	DIALOG_QUICK_SAVE,
	DIALOG_TIMER_ACCEPT_REVERT,
	DIALOG_CRAWL_SAVE,
	DIALOG_CONTINUE_LARGE,
	DIALOG_BENCHMARK,
};

/*
================================================
idDialogCallback

Generic engine-level callback interface for dialog buttons.
================================================
*/
class idDialogCallback
{
public:
	idDialogCallback() : refCount( 0 ) {}
	virtual ~idDialogCallback() {}

	void    AddRef()
	{
		refCount++;
	}
	void    Release()
	{
		if( --refCount <= 0 )
		{
			delete this;
		}
	}

	virtual void Call() = 0;

private:
	int refCount;
};

/*
================================================
idDialogInfo
================================================
*/
class idDialogInfo
{
public:
	idDialogInfo()
	{
		msg = GDM_INVALID;
		type = DIALOG_ACCEPT;
		acceptCB = NULL;
		cancelCB = NULL;
		altCBOne = NULL;
		altCBTwo = NULL;
		showing = false;
		clear = false;
		waitClear = false;
		pause = false;
		startTime = 0;
		killTime = 0;
		leaveOnClear = false;
		renderDuringLoad = false;
	}
	gameDialogMessages_t	msg;
	dialogType_t			type;
	idDialogCallback* 		acceptCB;
	idDialogCallback* 		cancelCB;
	idDialogCallback* 		altCBOne;
	idDialogCallback* 		altCBTwo;
	bool					showing;
	bool					clear;
	bool					waitClear;
	bool					pause;
	bool					forcePause;
	bool					leaveOnClear;
	bool					renderDuringLoad;
	int						startTime;
	int						killTime;
	idStrStatic< 256 >		overrideMsg;

	idStrId					txt1;
	idStrId					txt2;
	idStrId					txt3;
	idStrId					txt4;
};

class idGameDialogs
{
public:
	virtual						~idGameDialogs() {}

	virtual void				Init() = 0;
	virtual void				Shutdown() = 0;
	virtual void				Restart() = 0;

	virtual void				Render( bool loading ) = 0;

	virtual void				AddDialog( gameDialogMessages_t msg, dialogType_t type, idDialogCallback* acceptCallback, idDialogCallback* cancelCallback, bool pause, const char* location = NULL, int lineNumber = 0, bool leaveOnMapHeapReset = false, bool waitOnAtlas = false, bool renderDuringLoad = false ) = 0;
	virtual void				AddDynamicDialog( gameDialogMessages_t msg, const idStaticList< idDialogCallback*, 4 >& callbacks, const idStaticList< idStrId, 4 >& optionText, bool pause, idStrStatic< 256 > overrideMsg, bool leaveOnMapHeapReset = false, bool waitOnAtlas = false, bool renderDuringLoad = false ) = 0;

	virtual void				AddDialogIntVal( const char* name, int val ) = 0;

	virtual void				ClearDialog( gameDialogMessages_t msg, const char* location = NULL, int lineNumber = 0 ) = 0;
	virtual void				ClearDialogs( bool forceClear = false ) = 0;
	virtual void				ClearAllDialogHack() = 0;

	virtual bool				HasDialogMsg( gameDialogMessages_t msg, bool* isNowActive ) = 0;
	virtual bool				HasAnyActiveDialog() const = 0;
	virtual bool				IsDialogPausing() const = 0;

	virtual void				ShowSaveIndicator( bool show ) = 0;
	virtual bool				HandleDialogEvent( const sysEvent_t* sev ) = 0;
	virtual bool				IsDialogActive() const = 0;

	virtual idStr				GetDialogMsg( gameDialogMessages_t msg, idStr& outMessage, idStr& outTitle ) = 0;

	virtual bool				IsRendererLoaded() const = 0;
	virtual bool				IsRendererActive() const = 0;
	virtual void				ActivateRenderer( bool active ) = 0;

	virtual bool				IsSaveIndicatorActive() const = 0;
	virtual void				RenderDialog( int timeMicroseconds ) = 0;
	virtual void				RenderSaveIndicator( int timeMicroseconds ) = 0;

	virtual void				SetRendererGlobalInt( const char* name, int val ) = 0;
	virtual void				SetRendererGlobalString( const char* name, const char* val ) = 0;

	virtual void				AddRefCallback( idDialogCallback* cb ) = 0;
	virtual void				ReleaseCallback( idDialogCallback* cb ) = 0;
	virtual void				InvokeCallback( idDialogCallback* cb ) = 0;

	virtual void				BindDialogToRenderer( const idDialogInfo& info ) = 0;

	virtual void				AddDialogInternal( idDialogInfo& info ) = 0;
	virtual void				ShowDialog( const idDialogInfo& info ) = 0;
	virtual void				ShowNextDialog() = 0;
	virtual void				ActivateDialog( bool activate ) = 0;
	virtual void				RemoveWaitDialogs() = 0;
	virtual void				ReleaseCallBacks( int index ) = 0;
};

extern idGameDialogs*		dialogs;

/*
================================================================================================

	Leaderboards

================================================================================================
*/

struct lobbyUserID_t;
struct leaderboardStats_t;

class idLeaderboards
{
public:
	virtual						~idLeaderboards() {}

	// creates and stores all the leaderboards inside the internal map ( see Sys_FindLeaderboardDef on retrieving definition )
	virtual void				Init() = 0;

	// Destroys any leaderboard definitions allocated by Init()
	virtual void				Shutdown() = 0;

	// Gets a leaderboard ID with map index and game type.
	virtual int					GetID( int mapIndex, int gametype ) = 0;

	// Do it all function. Will create the column_t with the correct stats from the game type, and upload it to the leaderboard system.
	virtual void				Upload( lobbyUserID_t lobbyUserID, int gameType, leaderboardStats_t& stats ) = 0;
};

extern idLeaderboards* 		leaderBoards;

/*
===============================================================================

	Public game interface with methods for in-game editing.

===============================================================================
*/

typedef struct
{
	idSoundEmitter* 			referenceSound;	// this is the interface to the sound system, created
	// with idSoundWorld::AllocSoundEmitter() when needed
	idVec3						origin;
	int							listenerId;		// SSF_PRIVATE_SOUND only plays if == listenerId from PlaceListener
	// no spatialization will be performed if == listenerID
	const idSoundShader* 		shader;			// this really shouldn't be here, it is a holdover from single channel behavior
	float						diversity;		// 0.0 to 1.0 value used to select which
	// samples in a multi-sample list from the shader are used
	bool						waitfortrigger;	// don't start it at spawn time
	soundShaderParms_t			parms;			// override volume, flags, etc
} refSound_t;

enum
{
	TEST_PARTICLE_MODEL = 0,
	TEST_PARTICLE_IMPACT,
	TEST_PARTICLE_MUZZLE,
	TEST_PARTICLE_FLIGHT,
	TEST_PARTICLE_SELECTED
};

class idEntity;
class idMD5Anim;

class idGameEdit
{
public:
	virtual						~idGameEdit() {}

	// These are the canonical idDict to parameter parsing routines used by both the game and tools.
	virtual void				ParseSpawnArgsToRenderLight( const idDict* args, renderLight_t* renderLight ) = 0;
	virtual void				ParseSpawnArgsToRenderEntity( const idDict* args, renderEntity_t* renderEntity ) = 0;
	virtual void				ParseSpawnArgsToRenderEnvprobe( const idDict* args, renderEnvironmentProbe_t* renderEnvprobe ) = 0; // RB
	virtual void				ParseSpawnArgsToRefSound( const idDict* args, refSound_t* refSound ) = 0;

	// Animation system calls for non-game based skeletal rendering.
	virtual idRenderModel* 		ANIM_GetModelFromEntityDef( const char* classname ) = 0;
	virtual const idVec3&		ANIM_GetModelOffsetFromEntityDef( const char* classname ) = 0;
	virtual idRenderModel* 		ANIM_GetModelFromEntityDef( const idDict* args ) = 0;
	virtual idRenderModel* 		ANIM_GetModelFromName( const char* modelName ) = 0;
	virtual const idMD5Anim* 	ANIM_GetAnimFromEntityDef( const char* classname, const char* animname ) = 0;
	virtual int					ANIM_GetNumAnimsFromEntityDef( const idDict* args ) = 0;
	virtual const char* 		ANIM_GetAnimNameFromEntityDef( const idDict* args, int animNum ) = 0;
	virtual const idMD5Anim* 	ANIM_GetAnim( const char* fileName ) = 0;
	virtual int					ANIM_GetLength( const idMD5Anim* anim ) = 0;
	virtual int					ANIM_GetNumFrames( const idMD5Anim* anim ) = 0;
	virtual void				ANIM_CreateAnimFrame( const idRenderModel* model, const idMD5Anim* anim, int numJoints, idJointMat* frame, int time, const idVec3& offset, bool remove_origin_offset ) = 0;
	virtual idRenderModel* 		ANIM_CreateMeshForAnim( idRenderModel* model, const char* classname, const char* animname, int frame, bool remove_origin_offset ) = 0;

	// Articulated Figure calls for AF editor and Radiant.
	virtual bool				AF_SpawnEntity( const char* fileName ) = 0;
	virtual void				AF_UpdateEntities( const char* fileName ) = 0;
	virtual void				AF_UndoChanges() = 0;
	virtual idRenderModel* 		AF_CreateMesh( const idDict& args, idVec3& meshOrigin, idMat3& meshAxis, bool& poseIsSet ) = 0;


	// Entity selection.
	virtual void				ClearEntitySelection() = 0;
	virtual int					GetSelectedEntities( idEntity* list[], int max ) = 0;
	virtual void				AddSelectedEntity( idEntity* ent ) = 0;

	// Selection methods
	virtual void				TriggerSelected() = 0;

	// Entity defs and spawning.
	virtual const idDict* 		FindEntityDefDict( const char* name, bool makeDefault = true ) const = 0;
	virtual void				SpawnEntityDef( const idDict& args, idEntity** ent ) = 0;
	virtual idEntity* 			FindEntity( const char* name ) const = 0;
	virtual const char* 		GetUniqueEntityName( const char* classname ) const = 0;

	// Entity methods.
	virtual void				EntityGetOrigin( idEntity* ent, idVec3& org ) const = 0;
	virtual void				EntityGetAxis( idEntity* ent, idMat3& axis ) const = 0;
	virtual void				EntitySetOrigin( idEntity* ent, const idVec3& org ) = 0;
	virtual void				EntitySetAxis( idEntity* ent, const idMat3& axis ) = 0;
	virtual void				EntityTranslate( idEntity* ent, const idVec3& org ) = 0;
	virtual const idDict* 		EntityGetSpawnArgs( idEntity* ent ) const = 0;
	virtual void				EntityUpdateChangeableSpawnArgs( idEntity* ent, const idDict* dict ) = 0;
	virtual void				EntityChangeSpawnArgs( idEntity* ent, const idDict* newArgs ) = 0;
	virtual void				EntityUpdateVisuals( idEntity* ent ) = 0;
	virtual void				EntitySetModel( idEntity* ent, const char* val ) = 0;
	virtual void				EntityStopSound( idEntity* ent ) = 0;
	virtual void				EntityDelete( idEntity* ent ) = 0;
	virtual void				EntitySetColor( idEntity* ent, const idVec3 color ) = 0;

	// Player methods.
	virtual bool				PlayerIsValid() const = 0;
	virtual void				PlayerGetOrigin( idVec3& org ) const = 0;
	virtual void				PlayerGetAxis( idMat3& axis ) const = 0;
	virtual void				PlayerGetViewAngles( idAngles& angles ) const = 0;
	virtual void				PlayerGetEyePosition( idVec3& org ) const = 0;

	// In game map editing support.
	virtual const idDict* 		MapGetEntityDict( const char* name ) const = 0;
	virtual void				MapSave( const char* path = NULL ) const = 0;
	virtual void				MapSetEntityKeyVal( const char* name, const char* key, const char* val ) const  = 0;
	virtual void				MapCopyDictToEntity( const char* name, const idDict* dict ) const = 0;
	virtual int					MapGetUniqueMatchingKeyVals( const char* key, const char* list[], const int max ) const = 0;
	virtual void				MapAddEntity( const idDict* dict ) const = 0;
	virtual int					MapGetEntitiesMatchingClassWithString( const char* classname, const char* match, const char* list[], const int max ) const = 0;
	virtual void				MapRemoveEntity( const char* name ) const = 0;
	virtual void				MapEntityTranslate( const char* name, const idVec3& v ) const = 0;
};

extern idGameEdit* 				gameEdit;


/*
===============================================================================

	Game API.

===============================================================================
*/

const int GAME_API_VERSION		= 8;

typedef struct
{

	int							version;				// API version
	idSys* 						sys;					// non-portable system services
	idCommon* 					common;					// common
	idCmdSystem* 				cmdSystem;				// console command system
	idCVarSystem* 				cvarSystem;				// console variable system
	idFileSystem* 				fileSystem;				// file system
	idRenderSystem* 			renderSystem;			// render system
	idSoundSystem* 				soundSystem;			// sound system
	idRenderModelManager* 		renderModelManager;		// render model manager
	idUserInterfaceManager* 	uiManager;				// user interface manager
	idDeclManager* 				declManager;			// declaration manager
	idAASFileManager* 			AASFileManager;			// AAS file manager
	idCollisionModelManager* 	collisionModelManager;	// collision model manager

} gameImport_t;

typedef struct
{

	int							version;				// API version
	idGame* 					game;					// interface to run the game
	idGameEdit* 				gameEdit;				// interface for in-game editing
	idLeaderboards* 			leaderBoards;			// interface for leaderboards
	idGameMainMenu*				mainMenu;				// interface for the main menu
	idGameDialogs*				dialogs;				// interface for the gui dialogs

} gameExport_t;

extern "C" {
	typedef gameExport_t* ( *GetGameAPI_t )( gameImport_t* import );
}

#endif /* !__GAME_H__ */
