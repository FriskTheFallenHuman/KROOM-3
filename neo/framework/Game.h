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

#ifndef __GAME_H__
#define __GAME_H__

/*
===============================================================================

	Public game interface with methods to run the game.

===============================================================================
*/

typedef struct
{
	char		sessionCommand[MAX_STRING_CHARS];	// "map", "disconnect", "victory", etc
	int			consistencyHash;					// used to check for network game divergence
	int			health;
	int			heartRate;
	int			stamina;
	int			combat;
	bool		syncNextGameFrame;					// used when cinematics are skipped to prevent session from simulating several game frames to
	// keep the game time in sync with real time
} gameReturn_t;

typedef enum
{
	ALLOW_YES = 0,
	ALLOW_BADPASS,	// core will prompt for password and connect again
	ALLOW_NOTYET,	// core will wait with transmitted message
	ALLOW_NO		// core will abort with transmitted message
} allowReply_t;

typedef enum
{
	ESC_IGNORE = 0,	// do nothing
	ESC_MAIN,		// start main menu GUI
	ESC_GUI			// set an explicit GUI
} escReply_t;

class idGame
{
public:
	virtual						~idGame() {}

	// Initialize the game for the first time.
	virtual void				Init() = 0;

	// Shut down the entire game.
	virtual void				Shutdown() = 0;

	// Set the local client number. Distinguishes listen ( == 0 ) / dedicated ( == -1 )
	virtual void				SetLocalClient( int clientNum ) = 0;

	// Sets the user info for a client.
	// if canModify is true, the game can modify the user info in the returned dictionary pointer, server will forward the change back
	// canModify is never true on network client
	virtual const idDict* 		SetUserInfo( int clientNum, const idDict& userInfo, bool isClient, bool canModify ) = 0;

	// Retrieve the game's userInfo dict for a client.
	virtual const idDict* 		GetUserInfo( int clientNum ) = 0;

	// The game gets a chance to alter userinfo before they are emitted to server.
	virtual void				ThrottleUserInfo() = 0;

	// Sets the serverinfo at map loads and when it changes.
	virtual void				SetServerInfo( const idDict& serverInfo ) = 0;

	// The session calls this before moving the single player game to a new level.
	virtual const idDict& 		GetPersistentPlayerInfo( int clientNum ) = 0;

	// The session calls this right before a new level is loaded.
	virtual void				SetPersistentPlayerInfo( int clientNum, const idDict& playerInfo ) = 0;

	// Loads a map and spawns all the entities.
	virtual void				InitFromNewMap( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, bool isServer, bool isClient, int randseed, int activeEditors ) = 0;

	// Loads a map from a savegame file.
	virtual bool				InitFromSaveGame( const char* mapName, idRenderWorld* renderWorld, idSoundWorld* soundWorld, idFile* saveGameFile, int activeEditors ) = 0;

	// Saves the current game state, the session may have written some data to the file already.
	virtual void				SaveGame( idFile* saveGameFile ) = 0;

	// Shut down the current map.
	virtual void				MapShutdown() = 0;

	// Caches media referenced from in key/value pairs in the given dictionary.
	virtual void				CacheDictionaryMedia( const idDict* dict ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				SpawnPlayer( int clientNum ) = 0;

	// Runs a game frame, may return a session command for level changing, etc
	virtual gameReturn_t		RunFrame( const usercmd_t* clientCmds, int activeEditors ) = 0;

	// Makes rendering and sound system calls to display for a given clientNum.
	virtual bool				Draw( int clientNum ) = 0;

	// Let the game do it's own UI when ESCAPE is used
	virtual escReply_t			HandleESC( idUserInterface** gui ) = 0;

	// get the games menu if appropriate ( multiplayer )
	virtual idUserInterface* 	StartMenu() = 0;

	// When the game is running it's own UI fullscreen, GUI commands are passed through here
	// return NULL once the fullscreen UI mode should stop, or "main" to go to main menu
	virtual const char* 		HandleGuiCommands( const char* menuCommand ) = 0;

	// main menu commands not caught in the engine are passed here
	virtual void				HandleMainMenuCommands( const char* menuCommand, idUserInterface* gui ) = 0;

	// Early check to deny connect.
	virtual allowReply_t		ServerAllowClient( int numClients, const char* IP, const char* guid, const char* password, char reason[MAX_STRING_CHARS] ) = 0;

	// Connects a client.
	virtual void				ServerClientConnect( int clientNum, const char* guid ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				ServerClientBegin( int clientNum ) = 0;

	// Disconnects a client and removes the player entity from the game.
	virtual void				ServerClientDisconnect( int clientNum ) = 0;

	// Writes initial reliable messages a client needs to recieve when first joining the game.
	virtual void				ServerWriteInitialReliableMessages( int clientNum ) = 0;

	// Writes a snapshot of the server game state for the given client.
	virtual void				ServerWriteSnapshot( int clientNum, int sequence, idBitMsg& msg, byte* clientInPVS, int numPVSClients ) = 0;

	// Patches the network entity states at the server with a snapshot for the given client.
	virtual bool				ServerApplySnapshot( int clientNum, int sequence ) = 0;

	// Processes a reliable message from a client.
	virtual void				ServerProcessReliableMessage( int clientNum, const idBitMsg& msg ) = 0;

	// Reads a snapshot and updates the client game state.
	virtual void				ClientReadSnapshot( int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg& msg ) = 0;

	// Patches the network entity states at the client with a snapshot.
	virtual bool				ClientApplySnapshot( int clientNum, int sequence ) = 0;

	// Processes a reliable message from the server.
	virtual void				ClientProcessReliableMessage( int clientNum, const idBitMsg& msg ) = 0;

	// Runs prediction on entities at the client.
	virtual gameReturn_t		ClientPrediction( int clientNum, const usercmd_t* clientCmds, bool lastPredictFrame ) = 0;

	// Used to manage divergent time-lines
	virtual void				SelectTimeGroup( int timeGroup ) = 0;
	virtual int					GetTimeGroupTime( int timeGroup ) = 0;

	virtual void				GetBestGameType( const char* map, const char* gametype, char buf[ MAX_STRING_CHARS ] ) = 0;

	// Returns a summary of stats for a given client
	virtual void				GetClientStats( int clientNum, char* data, const int len ) = 0;

	// Switch a player to a particular team
	virtual void				SwitchTeam( int clientNum, int team ) = 0;

	virtual bool				DownloadRequest( const char* IP, const char* guid, const char* paks, char urls[ MAX_STRING_CHARS ] ) = 0;

	virtual void				GetMapLoadingGUI( char gui[ MAX_STRING_CHARS ] ) = 0;
};

extern idGame* 					game;


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
class idThread;
class function_t;
class idProgram;
class idInterpreter;
typedef struct prstack_s prstack_t;

// FIXME: this interface needs to be reworked but it properly separates code for the time being
class idGameEdit
{
public:
	virtual						~idGameEdit() {}

	// These are the canonical idDict to parameter parsing routines used by both the game and tools.
	virtual void				ParseSpawnArgsToRenderLight( const idDict* args, renderLight_t* renderLight );
	virtual void				ParseSpawnArgsToRenderEntity( const idDict* args, renderEntity_t* renderEntity );
	virtual void				ParseSpawnArgsToRenderEnvprobe( const idDict* args, renderEnvironmentProbe_t* renderEnvprobe ); // RB
	virtual void				ParseSpawnArgsToRefSound( const idDict* args, refSound_t* refSound );

	// Animation system calls for non-game based skeletal rendering.
	virtual idRenderModel* 		ANIM_GetModelFromEntityDef( const char* classname );
	virtual const idVec3&		 ANIM_GetModelOffsetFromEntityDef( const char* classname );
	virtual idRenderModel* 		ANIM_GetModelFromEntityDef( const idDict* args );
	virtual idRenderModel* 		ANIM_GetModelFromName( const char* modelName );
	virtual const idMD5Anim* 	ANIM_GetAnimFromEntityDef( const char* classname, const char* animname );
	virtual int					ANIM_GetNumAnimsFromEntityDef( const idDict* args );
	virtual const char* 		ANIM_GetAnimNameFromEntityDef( const idDict* args, int animNum );
	virtual const idMD5Anim* 	ANIM_GetAnim( const char* fileName );
	virtual int					ANIM_GetLength( const idMD5Anim* anim );
	virtual int					ANIM_GetNumFrames( const idMD5Anim* anim );
	virtual void				ANIM_CreateAnimFrame( const idRenderModel* model, const idMD5Anim* anim, int numJoints, idJointMat* frame, int time, const idVec3& offset, bool remove_origin_offset );
	virtual idRenderModel* 		ANIM_CreateMeshForAnim( idRenderModel* model, const char* classname, const char* animname, int frame, bool remove_origin_offset );

	// Articulated Figure calls for AF editor and Radiant.
	virtual bool				AF_SpawnEntity( const char* fileName );
	virtual void				AF_UpdateEntities( const char* fileName );
	virtual void				AF_UndoChanges();
	virtual idRenderModel* 		AF_CreateMesh( const idDict& args, idVec3& meshOrigin, idMat3& meshAxis, bool& poseIsSet );


	// Entity selection.
	virtual void				ClearEntitySelection();
	virtual int					GetSelectedEntities( idEntity* list[], int max );
	virtual void				AddSelectedEntity( idEntity* ent );

	// Selection methods
	virtual void				TriggerSelected();

	// Entity defs and spawning.
	virtual const idDict* 		FindEntityDefDict( const char* name, bool makeDefault = true ) const;
	virtual void				SpawnEntityDef( const idDict& args, idEntity** ent );
	virtual idEntity* 			FindEntity( const char* name ) const;
	virtual const char* 		GetUniqueEntityName( const char* classname ) const;

	// Entity methods.
	virtual void				EntityGetOrigin( idEntity* ent, idVec3& org ) const;
	virtual void				EntityGetAxis( idEntity* ent, idMat3& axis ) const;
	virtual void				EntitySetOrigin( idEntity* ent, const idVec3& org );
	virtual void				EntitySetAxis( idEntity* ent, const idMat3& axis );
	virtual void				EntityTranslate( idEntity* ent, const idVec3& org );
	virtual const idDict* 		EntityGetSpawnArgs( idEntity* ent ) const;
	virtual void				EntityUpdateChangeableSpawnArgs( idEntity* ent, const idDict* dict );
	virtual void				EntityChangeSpawnArgs( idEntity* ent, const idDict* newArgs );
	virtual void				EntityUpdateVisuals( idEntity* ent );
	virtual void				EntitySetModel( idEntity* ent, const char* val );
	virtual void				EntityStopSound( idEntity* ent );
	virtual void				EntityDelete( idEntity* ent );
	virtual void				EntitySetColor( idEntity* ent, const idVec3 color );

	// Player methods.
	virtual bool				PlayerIsValid() const;
	virtual void				PlayerGetOrigin( idVec3& org ) const;
	virtual void				PlayerGetAxis( idMat3& axis ) const;
	virtual void				PlayerGetViewAngles( idAngles& angles ) const;
	virtual void				PlayerGetEyePosition( idVec3& org ) const;

	// In game map editing support.
	virtual const idDict* 		MapGetEntityDict( const char* name ) const;
	virtual void				MapSave( const char* path = NULL ) const;
	virtual void				MapSetEntityKeyVal( const char* name, const char* key, const char* val ) const ;
	virtual void				MapCopyDictToEntity( const char* name, const idDict* dict ) const;
	virtual int					MapGetUniqueMatchingKeyVals( const char* key, const char* list[], const int max ) const;
	virtual void				MapAddEntity( const idDict* dict ) const;
	virtual int					MapGetEntitiesMatchingClassWithString( const char* classname, const char* match, const char* list[], const int max ) const;
	virtual void				MapRemoveEntity( const char* name ) const;
	virtual void				MapEntityTranslate( const char* name, const idVec3& v ) const;

	// In game script Debugging Support
	// IdProgram
	virtual void				GetLoadedScripts( idStrList** result );
	virtual bool				IsLineCode( const char* filename, int linenumber ) const;
	virtual const char* 		GetFilenameForStatement( idProgram* program, int index ) const;
	virtual int					GetLineNumberForStatement( idProgram* program, int index ) const;

	// idInterpreter
	virtual bool				CheckForBreakPointHit( const idInterpreter* interpreter, const function_t* function1, const function_t* function2, int depth ) const;
	virtual bool				ReturnedFromFunction( const idProgram* program, const idInterpreter* interpreter, int index ) const;
	virtual bool				GetRegisterValue( const idInterpreter* interpreter, const char* name, idStr& out, int scopeDepth ) const;
	virtual const idThread*		GetThread( const idInterpreter* interpreter ) const;
	virtual int					GetInterpreterCallStackDepth( const idInterpreter* interpreter );
	virtual const function_t*	GetInterpreterCallStackFunction( const idInterpreter* interpreter, int stackDepth = -1 );

	// IdThread
	virtual const char* 		ThreadGetName( const idThread* thread ) const;
	virtual int					ThreadGetNum( const idThread* thread ) const;
	virtual bool				ThreadIsDoneProcessing( const idThread* thread ) const;
	virtual bool				ThreadIsWaiting( const idThread* thread ) const;
	virtual bool				ThreadIsDying( const idThread* thread ) const;
	virtual int					GetTotalScriptThreads( ) const;
	virtual const idThread*		GetThreadByIndex( int index ) const;

	// MSG helpers
	virtual void				MSG_WriteThreadInfo( idBitMsg* msg, const idThread* thread, const idInterpreter* interpreter );
	virtual void				MSG_WriteCallstackFunc( idBitMsg* msg, const prstack_t* stack, const idProgram* program, int instructionPtr );
	virtual void				MSG_WriteInterpreterInfo( idBitMsg* msg, const idInterpreter* interpreter, const idProgram* program, int instructionPtr );
	virtual void				MSG_WriteScriptList( idBitMsg* msg );
};

extern idGameEdit* 				gameEdit;


/*
===============================================================================

	Game API.

===============================================================================
*/

const int GAME_API_VERSION		= 10;

typedef struct
{

	int							version;				// API version
	idSys* 						sys;					// non-portable system services
	idCommon* 					common;					// common
	idCmdSystem* 				cmdSystem;				// console command system
	idCVarSystem* 				cvarSystem;				// console variable system
	idFileSystem* 				fileSystem;				// file system
	idNetworkSystem* 			networkSystem;			// network system
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

} gameExport_t;

extern "C" {
	typedef gameExport_t* ( *GetGameAPI_t )( gameImport_t* import );
}

#endif /* !__GAME_H__ */
