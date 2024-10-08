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

#ifndef __SESSIONLOCAL_H__
#define __SESSIONLOCAL_H__

/*

IsConnectedToServer();
IsGameLoaded();
IsGuiActive();
IsPlayingRenderDemo();

if connected to a server
	if handshaking
	if map loading
	if in game
else if a game loaded
	if in load game menu
	if main menu up
else if playing render demo
else
	if error dialog
	full console

*/

typedef struct
{
	usercmd_t	cmd;
	int			consistencyHash;
} logCmd_t;

struct fileTIME_T
{
	int				index;
	ID_TIME_T			timeStamp;

	operator int() const
	{
		return timeStamp;
	}
};

typedef struct
{
	idDict			serverInfo;
	idDict			syncedCVars;
	idDict			userInfo[MAX_ASYNC_CLIENTS];
	idDict			persistentPlayerInfo[MAX_ASYNC_CLIENTS];
	usercmd_t		mapSpawnUsercmd[MAX_ASYNC_CLIENTS];		// needed for tracking delta angles
} mapSpawnData_t;

typedef enum
{
	TD_NO,
	TD_YES,
	TD_YES_THEN_QUIT
} timeDemo_t;

const int USERCMD_PER_DEMO_FRAME	= 2;
const int CONNECT_TRANSMIT_TIME		= 1000;
const int MAX_LOGGED_USERCMDS		= 60 * 60 * 60;	// one hour of single player, 15 minutes of four player

class idSessionLocal : public idSession
{
public:

	idSessionLocal();
	virtual				~idSessionLocal();

	virtual void		Init();

	virtual void		Shutdown();

	virtual void		Stop();

	// RB: added captureImage, swapBuffers parameters
	virtual void		UpdateScreen( bool captureImage, bool outOfSequence = true, bool swapBuffers = true );
	// RB end

	virtual void		PacifierUpdate();
	// foresthale 2014-05-30: a special binarize pacifier has to be shown in
	// some cases, which includes filename and ETA information, note that
	// the progress function takes 0-1 float, not 0-100, and can be called
	// very quickly (it will check that enough time has passed when updating)
	virtual void		PacifierBinarizeFilename( const char* filename, const char* reason );
	virtual void		PacifierBinarizeInfo( const char* info );
	virtual void		PacifierBinarizeMiplevel( int level, int maxLevel );
	virtual void		PacifierBinarizeProgress( float progress );
	virtual void		PacifierBinarizeEnd();
	// for images in particular we can measure more accurately this way (to deal with mipmaps)
	virtual void		PacifierBinarizeProgressTotal( int total );
	virtual void		PacifierBinarizeProgressIncrement( int step );

	virtual void		Frame();

	virtual bool		IsMultiplayer();

	virtual bool		ProcessEvent( const sysEvent_t* event );

	virtual void		StartMenu( bool playIntro = false );
	virtual void		ExitMenu();
	virtual void		GuiFrameEvents();
	virtual void		SetGUI( idUserInterface* gui, HandleGuiCommand_t handle );

	virtual const char* MessageBox( msgBoxType_t type, const char* message, const char* title = NULL, bool wait = false, const char* fire_yes = NULL, const char* fire_no = NULL, bool network = false );
	virtual void		StopBox();
	virtual void		DownloadProgressBox( backgroundDownload_t* bgl, const char* title, int progress_start = 0, int progress_end = 100 );
	virtual void		SetPlayingSoundWorld();

	virtual void		TimeHitch( int msec );

	virtual void		ReadCDKey();
	virtual void		WriteCDKey();
	virtual const char* GetCDKey( bool xp );
	virtual bool		CheckKey( const char* key, bool netConnect, bool offline_valid[ 2 ] );
	virtual bool		CDKeysAreValid( bool strict );
	virtual void		ClearCDKey( bool valid[ 2 ] );
	virtual void		SetCDKeyGuiVars();
	virtual bool		WaitingForGameAuth();
	virtual void		CDKeysAuthReply( bool valid, const char* auth_msg );

	virtual int			GetSaveGameVersion();

	// RB begin
	virtual idDemoFile* 		ReadDemo()
	{
		return readDemo;
	}
	virtual idDemoFile* 		WriteDemo()
	{
		return writeDemo;
	}

	virtual idRenderWorld* 		RW()
	{
		return rw;
	}
	virtual idSoundWorld* 		SW()
	{
		return sw;
	}
	// RB end

	virtual const char* GetCurrentMapName();

	//=====================================

	int					GetLocalClientNum();

	void				MoveToNewMap( const char* mapName );

	// loads a map and starts a new game on it
	void				StartNewGame( const char* mapName, bool devmap = false );
	void				PlayIntroGui();

	void				LoadSession( const char* name );
	void				SaveSession( const char* name );

	// called by Draw when the scene to scene wipe is still running
	void				DrawWipeModel();
	void				StartWipe( const char* materialName, bool hold = false );
	void				CompleteWipe();
	void				ClearWipe();

	void				ShowLoadingGui();

	void				ScrubSaveGameFileName( idStr& saveFileName ) const;
	idStr				GetAutoSaveName( const char* mapName ) const;

	bool				LoadGame( const char* saveName );
	// DG: added saveFileName so we can set a sensible filename for autosaves (see comment in MoveToNewMap())
	bool				SaveGame( const char* saveName, bool autosave = false, const char* saveFileName = NULL );

	bool				QuickSave();
	bool				QuickLoad();

	const char*			GetAuthMsg();

	//=====================================

	static idCVar		com_showAngles;
	static idCVar		com_showTics;
	static idCVar		com_minTics;
	static idCVar		com_showDemo;
	static idCVar		com_wipeSeconds;
	static idCVar		com_guid;
	static idCVar		com_numQuicksaves;

	static idCVar		gui_configServerRate;

	// RB begin
	// The render world and sound world used for this session.
	idRenderWorld* 		rw;
	idSoundWorld* 		sw;

	// The renderer	and sound system will write changes to writeDemo.
	// Demos can be	recorded and played at the same time when splicing.
	idDemoFile* 		readDemo;
	idDemoFile* 		writeDemo;
	int					renderdemoVersion;
	// RB end

	int					timeHitch;

	bool				menuActive;
	idSoundWorld* 		menuSoundWorld;			// so the game soundWorld can be muted

	bool				insideExecuteMapChange;	// draw loading screen and update
	// screen on prints
	int					bytesNeededForMapLoad;	//

	// we don't want to redraw the loading screen for every single
	// console print that happens
	int					lastPacifierTime;

	// foresthale 2014-05-30: a special binarize pacifier has to be shown in some cases, which includes filename and ETA information
	bool				pacifierBinarizeActive;
	int					pacifierBinarizeStartTime;
	float				pacifierBinarizeProgress;
	float				pacifierBinarizeTimeLeft;
	idStr				pacifierBinarizeFilename;
	idStr				pacifierBinarizeInfo;
	int					pacifierBinarizeMiplevel;
	int					pacifierBinarizeMiplevelTotal;
	int					pacifierBinarizeProgressTotal;
	int					pacifierBinarizeProgressCurrent;

	// this is the information required to be set before ExecuteMapChange() is called,
	// which can be saved off at any time with the following commands so it can all be played back
	mapSpawnData_t		mapSpawnData;
	idStr				currentMapName;			// for checking reload on same level
	bool				mapSpawned;				// cleared on Stop()

	int					numClients;				// from serverInfo

	int					logIndex;
	logCmd_t			loggedUsercmds[MAX_LOGGED_USERCMDS];
	int					statIndex;
	logStats_t			loggedStats[MAX_LOGGED_STATS];
	int					lastSaveIndex;
	// each game tic, numClients usercmds will be added, until full

	bool				insideUpdateScreen;	// true while inside ::UpdateScreen()

	bool				loadingSaveGame;	// currently loading map from a SaveGame
	idFile* 			savegameFile;		// this is the savegame file to load from
	int					savegameVersion;

	idFile* 			cmdDemoFile;		// if non-zero, we are reading commands from a file

	int					latchedTicNumber;	// set to com_ticNumber each frame
	int					lastGameTic;		// while latchedTicNumber > lastGameTic, run game frames
	int					lastDemoTic;

	// RB begin
	int					gameFrame;					// local game frame
	int					gameTime;					// local game time
	double				gameTimeResidual;			// left over time from previous frame
	// RB end

	bool				syncNextGameFrame;


	bool				aviCaptureMode;		// if true, screenshots will be taken and sound captured
	idStr				aviDemoShortName;	//
	float				aviDemoFrameCount;
	int					aviTicStart;

	timeDemo_t			timeDemo;
	int					timeDemoStartTime;
	int					numDemoFrames;		// for timeDemo and demoShot
	int					demoTimeOffset;
	renderView_t		currentDemoRenderView;
	// the next one will be read when
	// com_frameTime + demoTimeOffset > currentDemoRenderView.

	// TODO: make this private (after sync networking removal and idnet tweaks)
	idUserInterface* 	guiActive;
	HandleGuiCommand_t	guiHandle;

	idUserInterface* 	guiInGame;
	idUserInterface* 	guiMainMenu;
	idListGUI* 			guiMainMenu_MapList;		// easy map list handling
	idUserInterface* 	guiRestartMenu;
	idUserInterface* 	guiLoading;
	idUserInterface* 	guiIntro;
	idUserInterface* 	guiTest;

	idUserInterface* 	guiMsg;
	idUserInterface* 	guiMsgRestore;				// store the calling GUI for restore
	idStr				msgFireBack[ 2 ];
	bool				msgRunning;
	int					msgRetIndex;
	bool				msgIgnoreButtons;

	bool				waitingOnBind;

	const idMaterial* 	whiteMaterial;

	const idMaterial* 	wipeMaterial;
	int					wipeStartTic;
	int					wipeStopTic;
	bool				wipeHold;

#if ID_CONSOLE_LOCK
	int					emptyDrawCount;				// watchdog to force the main menu to restart
#endif

	//=====================================
	void				Clear();

	void				DrawCmdGraph();
	void				Draw();

	void				WriteCmdDemo( const char* name, bool save = false );
	void				StartPlayingCmdDemo( const char* demoName );
	void				TimeCmdDemo( const char* demoName );
	void				SaveCmdDemoToFile( idFile* file );
	void				LoadCmdDemoFromFile( idFile* file );
	void				StartRecordingRenderDemo( const char* name );
	void				StopRecordingRenderDemo();
	void				StartPlayingRenderDemo( idStr name );
	void				StopPlayingRenderDemo();
	void				CompressDemoFile( const char* scheme, const char* name );
	void				TimeRenderDemo( const char* name, bool twice = false, bool quit = false );
	void				AVIRenderDemo( const char* name );
	void				AVICmdDemo( const char* name );
	void				AVIGame( const char* name );
	void				BeginAVICapture( const char* name );
	void				EndAVICapture();

	void				AdvanceRenderDemo( bool singleFrameOnly );
	void				RunGameTic();

	void				FinishCmdLoad();
	void				LoadLoadingGui( const char* mapName );

	void				DemoShot( const char* name );

	void				TestGUI( const char* name );

	int					GetBytesNeededForMapLoad( const char* mapName );
	void				SetBytesNeededForMapLoad( const char* mapName, int bytesNeeded );

	void				ExecuteMapChange( bool noFadeWipe = false );
	void				UnloadMap();

	// return true if we actually waiting on an auth reply
	bool				MaybeWaitOnCDKey();

	//------------------
	// Session_menu.cpp

	idStrList			loadGameList;
	idStrList			modsList;

	idUserInterface* 	GetActiveMenu();

	void				DispatchCommand( idUserInterface* gui, const char* menuCommand, bool doIngame = true );
	void				MenuEvent( const sysEvent_t* event );
	bool				HandleSaveGameMenuCommand( idCmdArgs& args, int& icmd );
	void				HandleInGameCommands( const char* menuCommand );
	void				HandleMainMenuCommands( const char* menuCommand );
	void				HandleChatMenuCommands( const char* menuCommand );
	void				HandleIntroMenuCommands( const char* menuCommand );
	void				HandleRestartMenuCommands( const char* menuCommand );
	void				HandleMsgCommands( const char* menuCommand );
	void				GetSaveGameList( idStrList& fileList, idList<fileTIME_T>& fileTimes );
	void				UpdateMPLevelShot();

	void				SetSaveGameGuiVars();
	void				SetMainMenuGuiVars();
	void				SetModsMenuGuiVars();
	void				SetMainMenuSkin();
	void				SetPbMenuGuiVars();

private:
	bool				BoxDialogSanityCheck();
	void				EmitGameAuth();

	typedef enum
	{
		CDKEY_UNKNOWN,	// need to perform checks on the key
		CDKEY_INVALID,	// that key is wrong
		CDKEY_OK,		// valid
		CDKEY_CHECKING, // sent a check request ( gameAuth only )
		CDKEY_NA		// does not apply, xp key when xp is not present
	} cdKeyState_t;

	static const int	CDKEY_BUF_LEN = 17;
	static const int	CDKEY_AUTH_TIMEOUT = 5000;

	char				cdkey[ CDKEY_BUF_LEN ];
	cdKeyState_t		cdkey_state;
	char				xpkey[ CDKEY_BUF_LEN ];
	cdKeyState_t		xpkey_state;
	int					authEmitTimeout;
	bool				authWaitBox;

	idStr				authMsg;

	bool				demoversion; // DG: true if running the Demo version of Doom3, for FT_IsDemo (see Common.h)
};

extern idSessionLocal	sessLocal;

#endif /* !__SESSIONLOCAL_H__ */
