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

#ifndef __SESSION_H__
#define __SESSION_H__

/*
===============================================================================

	The session is the glue that holds games together between levels.

===============================================================================
*/

// needed by the gui system for the load game menu
typedef struct
{
	short		health;
	short		heartRate;
	short		stamina;
	short		combat;
} logStats_t;

static const int	MAX_LOGGED_STATS = 60 * 120;		// log every half second

typedef enum
{
	MSG_OK,
	MSG_ABORT,
	MSG_OKCANCEL,
	MSG_YESNO,
	MSG_PROMPT,
	MSG_CDKEY,
	MSG_INFO,
	MSG_WAIT
} msgBoxType_t;

typedef const char* ( *HandleGuiCommand_t )( const char* );

#ifdef MessageBox
	#undef MessageBox
#endif

class idSession
{
public:
	virtual			~idSession() {}

	// Called in an orderly fashion at system startup,
	// so commands, cvars, files, etc are all available.
	virtual	void	Init() = 0;

	// Shut down the session.
	virtual	void	Shutdown() = 0;

	// Called on errors and game exits.
	virtual void	Stop() = 0;

	// Redraws the screen, handling games, guis, console, etc
	// during normal once-a-frame updates, outOfSequence will be false,
	// but when the screen is updated in a modal manner, as with utility
	// output, the mouse cursor will be released if running windowed.
	// RB: added swapBuffers parameter
	virtual void	UpdateScreen( bool captureToImage, bool outOfSequence = true, bool swapBuffers = true ) = 0;
	// RB end

	// Called when console prints happen, allowing the loading screen
	// to redraw if enough time has passed.
	virtual void	PacifierUpdate() = 0;

	// Called every frame, possibly spinning in place if we are
	// above maxFps, or we haven't advanced at least one demo frame.
	// Returns the number of milliseconds since the last frame.
	virtual void	Frame() = 0;

	// Returns true if a multiplayer game is running.
	// CVars and commands are checked differently in multiplayer mode.
	virtual bool	IsMultiplayer() = 0;

	// Processes the given event.
	virtual	bool	ProcessEvent( const sysEvent_t* event ) = 0;

	// Activates the main menu
	virtual void	StartMenu( bool playIntro = false ) = 0;

	virtual void	SetGUI( idUserInterface* gui, HandleGuiCommand_t handle ) = 0;

	// Updates gui and dispatched events to it
	virtual void	GuiFrameEvents() = 0;

	// fires up the optional GUI event, also returns them if you set wait to true
	// if MSG_PROMPT and wait, returns the prompt string or NULL if aborted
	// if MSG_CDKEY and want, returns the cd key or NULL if aborted
	// network tells wether one should still run the network loop in a wait dialog
	virtual const char* MessageBox( msgBoxType_t type, const char* message, const char* title = NULL, bool wait = false, const char* fire_yes = NULL, const char* fire_no = NULL, bool network = false ) = 0;
	virtual void	StopBox() = 0;

	// monitor this download in a progress box to either abort or completion
	virtual void	DownloadProgressBox( backgroundDownload_t* bgl, const char* title, int progress_start = 0, int progress_end = 100 ) = 0;

	virtual void	SetPlayingSoundWorld() = 0;

	// this is used by the sound system when an OnDemand sound is loaded, so the game action
	// doesn't advance and get things out of sync
	virtual void	TimeHitch( int msec ) = 0;

	// read and write the cd key data to files
	// doesn't perform any validity checks
	virtual void	ReadCDKey() = 0;
	virtual void	WriteCDKey() = 0;

	// returns NULL for if xp is true and xp key is not valid or not present
	virtual const char* GetCDKey( bool xp ) = 0;

	// check keys for validity when typed in by the user ( with checksum verification )
	// store the new set of keys if they are found valid
	virtual bool	CheckKey( const char* key, bool netConnect, bool offline_valid[ 2 ] ) = 0;

	// verify the current set of keys for validity
	// strict -> keys in state CDKEY_CHECKING state are not ok
	virtual bool	CDKeysAreValid( bool strict ) = 0;
	// wipe the key on file if the network check finds it invalid
	virtual void	ClearCDKey( bool valid[ 2 ] ) = 0;

	// configure gui variables for mainmenu.gui and cd key state
	virtual void	SetCDKeyGuiVars() = 0;

	virtual bool	WaitingForGameAuth() = 0;

	// got reply from master about the keys. if !valid, auth_msg given
	virtual void	CDKeysAuthReply( bool valid, const char* auth_msg ) = 0;

	virtual const char* GetCurrentMapName() = 0;

	virtual int		GetSaveGameVersion() = 0;

	// RB begin
	virtual idDemoFile* 		ReadDemo() = 0;
	virtual idDemoFile* 		WriteDemo() = 0;

	virtual idRenderWorld* 		RW() = 0;
	virtual idSoundWorld* 		SW() = 0;
	// RB end
};

extern	idSession* 	session;

#endif /* !__SESSION_H__ */
