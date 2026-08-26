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

#ifndef __GAME_MAINMENU_H__
#define	__GAME_MAINMENU_H__

typedef enum
{
	MENU_SCREEN_NONE,
	MENU_SCREEN_MAIN,
	MENU_SCREEN_PAUSE,
	MENU_SCREEN_INTRO,
	MENU_SCREEN_LOADING,
	MENU_SCREEN_RESTART,
	MENU_SCREEN_ONLINE_STATUS
} gameUIScreenStates_t;


/*
===============================================================================

	Local implementation of the public main menu interface.

===============================================================================
*/

class idGameMainMenuLocal : public idGameMainMenu
{
public:
	// ---------------------- Public idGameMainMenu Interface -------------------

	idGameMainMenuLocal();

	virtual void				Initialize();
	virtual void				Shutdown();

	virtual void				Init( const char* filename, idSoundWorld* sw );
	virtual void				InitMenu();
	virtual bool				IsLoadingActive() const;
	virtual void				LoadingGui( const char* mapName, bool& hellMap );
	virtual void				RenderLoadingShell();
	virtual void				Cleanup( bool onlyLoading = false );
	virtual void				CreateMenu( bool inGame );
	virtual void				ClosePause();
	virtual void				Show( bool show );
	virtual bool				IsActive() const;
	virtual bool				HandleGuiEvent( const sysEvent_t* sev );
	virtual void				Render();
	virtual void				ResetMenu();
	virtual void				SyncWithSession();
	virtual void				UpdateSavedGames();
	virtual void				SetCanContinue( bool valid );
	virtual void				UpdateClientCountdown( int countdown );
	virtual void				UpdateLeaderboard( const idLeaderboardCallback* callback );
	virtual void				SetGameComplete();
	virtual bool				IsShowingIntro();
	virtual bool				IsGameComplete();

	// ---------------------- Public idGameMainMenuLocal Interface -------------------
	void						ClearRepeater() {}

private:

	void						SetGUI( idUserInterface* gui );
	void						ExitMenu();
	void						DispatchCommand( idUserInterface* gui, const char* menuCommand );

	bool						HandleSaveGameMenuCommand( idCmdArgs& args, int& icmd );
	void						HandleMainMenuCommands( const char* menuCommand );
	void						HandleInGameCommands( const char* menuCommand );
	void						HandleRestartMenuCommands( const char* menuCommand );
	void						HandleIntroMenuCommands( const char* menuCommand );

	void						SetVideoGuiVars();
	void						SetSaveGameGuiVars();
	void						SetMainMenuGuiVars();
	void						GetSaveGameList();

	idUserInterface*			guiActive;
	idUserInterface*			guiMainMenu;
	idUserInterface*			guiRestartMenu;
	idUserInterface*			guiIntro;
	idUserInterface* 			guiIntroD3;
	idUserInterface* 			guiIntroD3XP;
	idUserInterface* 			guiIntroD3LE;
	//idUserInterface*			guiOnlineStatus;
	idUserInterface*			guiLoading;

	saveGameDetailsList_t		sortedSaves;

	idListGUI*					saveGameListGUI;

	idSoundWorld*				sw;
	idSoundWorld*				menuSW;

	gameUIScreenStates_t			currentScreen;
	bool						inGame;
	bool						canContinue;
	bool						gameComplete;
	bool						showingIntro;
	idStr						introMapName;
	int							loadStartTime;
	int							expansionType;	// 0 = D3, 1 = D3XP, 2 = D3LE
};

#endif /* !__GAME_MAINMENU_H__ */
