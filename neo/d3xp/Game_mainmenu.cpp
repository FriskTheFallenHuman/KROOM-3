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

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"

extern idCVar popupDialog_debug;

idGameMainMenuLocal			mainMenuLocal;
idGameMainMenu* 				mainMenu = &mainMenuLocal;

namespace GameUIUtils
{
idStr GameUI_EnableDisableStr()
{
	idStr result = idLocalization::GetString( "#str_swf_disabled" );
	result += ";";
	result += idLocalization::GetString( "#str_swf_enabled" );

	return result;
}
};

/*
========================
idGameMainMenuLocal::idGameMainMenuLocal
========================
*/
idGameMainMenuLocal::idGameMainMenuLocal()
{
	guiActive = NULL;
	guiMainMenu = NULL;
	guiRestartMenu = NULL;
	guiIntro = NULL;
	guiIntroD3 = NULL;
	guiIntroD3XP = NULL;
	guiIntroD3LE = NULL;
	//guiOnlineStatus = NULL;
	guiLoading = NULL;
	saveGameListGUI = NULL;
	sw = NULL;
	menuSW = NULL;
	currentScreen = MENU_SCREEN_NONE;
	inGame = false;
	canContinue = false;
	gameComplete = false;
	showingIntro = false;
	introMapName = "game/mars_city1";
	loadStartTime = 0;
	expansionType = 0;
}

/*
========================
idGameMainMenuLocal::Initialize
========================
*/
void idGameMainMenuLocal::Initialize()
{
	guiMainMenu		= uiManager->FindGui( "guis/mainmenu.gui", true, false, true );
	guiRestartMenu	= uiManager->FindGui( "guis/restart.gui", true, false, true );
	guiIntroD3		= uiManager->FindGui( "guis/intro.gui", true, false, true );
	guiIntro		= guiIntroD3;
	guiIntroD3XP	= uiManager->FindGui( "guis/d3xp-intro.gui", true, false, true );
	guiIntroD3LE	= uiManager->FindGui( "guis/d3le-intro.gui", true, false, true );

	//guiOnlineStatus = uiManager->CheckGui( "guis/online_status.gui" )
	//				  ? uiManager->FindGui( "guis/online_status.gui", true, false, true )
	//				  : NULL;

	if( saveGameListGUI == NULL )
	{
		saveGameListGUI = uiManager->AllocListGUI();
	}
	if( guiMainMenu != NULL && saveGameListGUI != NULL )
	{
		saveGameListGUI->Config( guiMainMenu, "loadGame" );
	}
}

/*
========================
idGameMainMenuLocal::Shutdown
========================
*/
void idGameMainMenuLocal::Shutdown()
{
	Cleanup( false );

	sw = NULL;
	menuSW = NULL;

	if( saveGameListGUI != NULL )
	{
		saveGameListGUI->Shutdown();
		uiManager->FreeListGUI( saveGameListGUI );
		saveGameListGUI = NULL;
	}
}

/*
========================
idGameMainMenuLocal::SetGUI
========================
*/
void idGameMainMenuLocal::SetGUI( idUserInterface* gui )
{
	guiActive = gui;
	if( guiActive == NULL )
	{
		return;
	}

	if( guiActive == guiMainMenu )
	{
		SetSaveGameGuiVars();
		SetMainMenuGuiVars();
	}
	else if( guiActive == guiRestartMenu )
	{
		SetSaveGameGuiVars();
	}

	sysEvent_t ev;
	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_NONE;
	guiActive->HandleEvent( &ev, Sys_Milliseconds() );
	guiActive->Activate( true, Sys_Milliseconds() );

	if( guiActive == guiMainMenu )
	{
		guiActive->SetStateFloat( "activate2", 0.0f );
		guiActive->SetStateString( "StartBlackFade::backcolor", inGame ? "0 0 0 1" : "0 0 0 0" );
		guiActive->HandleNamedEvent( "noIntro" );
		guiActive->StateChanged( Sys_Milliseconds() );

		guiActive->SetStateString( "game_list", idLocalization::GetString( "#str_07212" ) );
	}

	if( sw != NULL && !sw->IsPaused() )
	{
		sw->Pause();
	}
	soundSystem->SetPlayingSoundWorld( menuSW );
}

/*
========================
idGameMainMenuLocal::ExitMenu
========================
*/
void idGameMainMenuLocal::ExitMenu()
{
	guiActive = NULL;

	if( sw != NULL && ( common == NULL || !common->IsShuttingDown() ) )
	{
		// go back to the game sounds
		soundSystem->SetPlayingSoundWorld( sw );

		// unpause the game sound world
		if( sw->IsPaused() )
		{
			sw->UnPause();
		}
	}
}

/*
========================
idGameMainMenuLocal::Init
========================
*/
void idGameMainMenuLocal::Init( const char* filename, idSoundWorld* _sw )
{
	sw = _sw;
	menuSW = common->MenuSW();
	currentScreen = inGame ? MENU_SCREEN_PAUSE : MENU_SCREEN_MAIN;
	SetGUI( guiMainMenu );
}

/*
========================
idGameMainMenuLocal::InitMenu
========================
*/
void idGameMainMenuLocal::InitMenu()
{
	declManager->BeginLevelLoad();
	renderSystem->BeginLevelLoad();
	soundSystem->BeginLevelLoad();
	uiManager->BeginLevelLoad();

	Initialize();

	CreateMenu( false );
	Show( true );
	SyncWithSession();

	renderSystem->EndLevelLoad();
	soundSystem->EndLevelLoad();
	declManager->EndLevelLoad();
	uiManager->EndLevelLoad( "" );
}

/*
========================
idGameMainMenuLocal::IsLoadingActive
========================
*/
bool idGameMainMenuLocal::IsLoadingActive() const
{
	return guiLoading != NULL && guiLoading->IsActive();
}

/*
========================
idGameMainMenuLocal::LoadingGui
========================
*/
void idGameMainMenuLocal::LoadingGui( const char* mapName, bool& hellMap )
{
	Initialize();

	idStrStatic< MAX_OSPATH > stripped = mapName;
	stripped.StripFileExtension();
	stripped.StripPath();

	idStrStatic< MAX_OSPATH > guiMap = "guis/map/";
	guiMap.Append( stripped );
	guiMap.Append( ".gui" );

	if( uiManager->CheckGui( guiMap ) )
	{
		guiLoading = uiManager->FindGui( guiMap, true, false, true );
	}
	else
	{
		guiLoading = uiManager->FindGui( "guis/map/loading.gui", true, false, true );
	}

	if( guiLoading == NULL )
	{
		hellMap = false;
		return;
	}

	guiLoading->SetStateFloat( "map_loading", 0.0f );

	loadStartTime = Sys_Milliseconds();

	const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_MAPDEF, mapName, false ) );
	hellMap = ( mapDef != NULL ) && mapDef->dict.GetBool( "hellMap", false );
	if( mapDef != NULL )
	{
		idStr displayName = idLocalization::GetString( mapDef->dict.GetString( "name", mapName ) );
		guiLoading->SetStateString( "mapName", displayName );
		guiLoading->SetStateString( "mapDesc", idLocalization::GetString( mapDef->dict.GetString( "desc", "" ) ) );
	}

	guiLoading->Activate( true, Sys_Milliseconds() );
}

/*
========================
idGameMainMenuLocal::RenderLoadingShell
========================
*/
void idGameMainMenuLocal::RenderLoadingShell()
{
	if( IsLoadingActive() )
	{
		static const int ESTIMATED_LOAD_MS = 8000;
		float pct = ( float )( Sys_Milliseconds() - loadStartTime ) / ( float )ESTIMATED_LOAD_MS;
		guiLoading->SetStateFloat( "map_loading", pct );
		guiLoading->StateChanged( Sys_Milliseconds() );

		guiLoading->Redraw( Sys_Milliseconds() );
	}
}

/*
========================
idGameMainMenuLocal::Cleanup
========================
*/
void idGameMainMenuLocal::Cleanup( bool onlyLoading )
{
	if( guiLoading != NULL )
	{
		guiLoading = NULL;
	}

	if( !onlyLoading )
	{
		ExitMenu();
	}
}

/*
========================
idGameMainMenuLocal::CreateMenu
========================
*/
void idGameMainMenuLocal::CreateMenu( bool _inGame )
{
	ResetMenu();

	inGame = _inGame;

	if( !inGame )
	{
		Init( "shell", common->MenuSW() );
	}
	else
	{
		Init( "pause", common->SW() );

		if( guiActive != NULL )
		{
			guiActive->Activate( false, Sys_Milliseconds() );
			guiActive = NULL;
		}

	}
}

/*
========================
idGameMainMenuLocal::ClosePause
========================
*/
void idGameMainMenuLocal::ClosePause()
{
	if( !common->IsMultiplayer() && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->health <= 0 )
	{
		return;
	}

	if( gameComplete )
	{
		return;
	}

	ExitMenu();
}

/*
========================
idGameMainMenuLocal::Show
========================
*/
void idGameMainMenuLocal::Show( bool show )
{
	if( show )
	{
		bool isDead = false;
		if( inGame && !common->IsMultiplayer() )
		{
			if( !common->IsMultiplayer() )
			{
				idPlayer* player = gameLocal.GetLocalPlayer();
				isDead = player != NULL && player->health <= 0;
			}
		}

		if( inGame && !common->IsMultiplayer() && ( isDead || gameComplete ) && guiRestartMenu != NULL )
		{
			currentScreen = MENU_SCREEN_RESTART;
			SetGUI( guiRestartMenu );
		}
		else
		{
			currentScreen = inGame ? MENU_SCREEN_PAUSE : MENU_SCREEN_MAIN;
			SetGUI( guiMainMenu );
		}
	}
	else
	{
		ExitMenu();
	}
}

/*
========================
idGameMainMenuLocal::IsActive
========================
*/
bool idGameMainMenuLocal::IsActive() const
{
	return guiActive != NULL;
}

/*
========================
idGameMainMenuLocal::HandleGuiEvent
========================
*/
bool idGameMainMenuLocal::HandleGuiEvent( const sysEvent_t* sev )
{
	if( guiActive == NULL )
	{
		return false;
	}

	if( sev->evType == SE_MOUSE_ABSOLUTE )
	{
		const float width = renderSystem->GetWidth();
		const float height = renderSystem->GetHeight();
		if( width > 0.0f && height > 0.0f )
		{
			guiActive->SetCursor( ( float )sev->evValue * SCREEN_WIDTH / width,
								  ( float )sev->evValue2 * SCREEN_HEIGHT / height );
		}
	}

	const char*	menuCommand = guiActive->HandleEvent( sev, Sys_Milliseconds() );

	if( !menuCommand || !menuCommand[0] )
	{
		// If the menu didn't handle the event, and it's a key down event for an F key, run the bind
		if( sev->evType == SE_KEY && sev->evValue2 == 1 && sev->evValue >= K_F1 && sev->evValue <= K_F12 )
		{
			idKeyInput::ExecKeyBinding( sev->evValue );
		}
		return false;
	}

	DispatchCommand( guiActive, menuCommand );
	return true;
}

/*
========================
idGameMainMenuLocal::Render
========================
*/
void idGameMainMenuLocal::Render()
{
	if( guiActive != NULL )
	{
		guiActive->Redraw( Sys_Milliseconds() );

		sysEvent_t ev;
		memset( &ev, 0, sizeof( ev ) );
		ev.evType = SE_NONE;
		const char* menuCommand = guiActive->HandleEvent( &ev, Sys_Milliseconds() );
		if( menuCommand != NULL && menuCommand[0] != '\0' )
		{
			DispatchCommand( guiActive, menuCommand );
		}
	}
}

/*
========================
idGameMainMenuLocal::ResetMenu
========================
*/
void idGameMainMenuLocal::ResetMenu()
{
	ExitMenu();
	currentScreen = MENU_SCREEN_NONE;
}

/*
========================
idGameMainMenuLocal::SyncWithSession
========================
*/
void idGameMainMenuLocal::SyncWithSession()
{
	switch( session->GetState() )
	{
		case idSession::INGAME:
		{
			if( showingIntro )
			{
				showingIntro = false;
				ExitMenu();
			}

			if( guiActive == guiRestartMenu )
			{
				currentScreen = MENU_SCREEN_RESTART;
			}
			else
			{
				currentScreen = MENU_SCREEN_PAUSE;
			}
			break;
		}
		case idSession::IDLE:
			currentScreen = MENU_SCREEN_MAIN;
			break;
		case idSession::CONNECTING:
			currentScreen = MENU_SCREEN_ONLINE_STATUS;
			//if( guiOnlineStatus != NULL )
			//{
			//	guiOnlineStatus->SetStateString( "status_text", idLocalization::GetString( "#str_dlg_connecting" ) );
			//	SetGUI( guiOnlineStatus );
			//}
			break;
		case idSession::SEARCHING:
			currentScreen = MENU_SCREEN_ONLINE_STATUS;
			//if( guiOnlineStatus != NULL )
			//{
			//	guiOnlineStatus->SetStateString( "status_text", idLocalization::GetString( "#str_online_mpstatus_searching" ) );
			//	SetGUI( guiOnlineStatus );
			//}
			break;
		case idSession::PARTY_LOBBY:
		case idSession::GAME_LOBBY:
		case idSession::BUSY:
			currentScreen = MENU_SCREEN_ONLINE_STATUS;
			//if( guiOnlineStatus != NULL )
			//{
			//	guiOnlineStatus->SetStateString( "status_text", "..." );	// TODO: real per-state text
			//	SetGUI( guiOnlineStatus );
			//}
			break;
		case idSession::LOADING:
			currentScreen = MENU_SCREEN_LOADING;
			break;
		default:
			break;
	}
}

/*
========================
idGameMainMenuLocal::UpdateSavedGames
========================
*/
void idGameMainMenuLocal::UpdateSavedGames()
{
	SetSaveGameGuiVars();
}

/*
========================
idGameMainMenuLocal::SetCanContinue
========================
*/
void idGameMainMenuLocal::SetCanContinue( bool valid )
{
	canContinue = valid;

	if( guiMainMenu != NULL )
	{
		guiMainMenu->SetStateBool( "canContinue", valid );
	}
}

/*
========================
idGameMainMenuLocal::UpdateClientCountdown
========================
*/
void idGameMainMenuLocal::UpdateClientCountdown( int countdown )
{
#if 0
	if( guiOnlineStatus != NULL )
	{
		guiOnlineStatus->SetStateInt( "countdown", countdown );
	}
#endif
}

/*
========================
idGameMainMenuLocal::UpdateLeaderboard
========================
*/
void idGameMainMenuLocal::UpdateLeaderboard( const idLeaderboardCallback* callback )
{
#if 0
	if( guiOnlineStatus == NULL || callback == NULL )
	{
		return;
	}

	if( callback->GetErrorCode() != LEADERBOARD_ERROR_NONE )
	{
		guiOnlineStatus->SetStateString( "status_text", "Leaderboard error" );
		return;
	}

	const idList< idLeaderboardCallback::row_t >& rows = callback->GetRows();
	const leaderboardDefinition_t* def = callback->GetDef();

	static const int ROWS_SHOWN = 10;
	for( int i = 0; i < ROWS_SHOWN; i++ )
	{
		if( i < rows.Num() )
		{
			const idLeaderboardCallback::row_t& row = rows[i];
			guiOnlineStatus->SetStateString( va( "lb_name_%i", i ), row.name.c_str() );
			guiOnlineStatus->SetStateInt( va( "lb_rank_%i", i ), ( int )row.rank );
			if( def != NULL && def->numColumns > 0 )
			{
				guiOnlineStatus->SetStateInt( va( "lb_score_%i", i ), ( int )row.columns[0] );
			}
		}
		else
		{
			guiOnlineStatus->DeleteStateVar( va( "lb_name_%i", i ) );
			guiOnlineStatus->DeleteStateVar( va( "lb_rank_%i", i ) );
			guiOnlineStatus->DeleteStateVar( va( "lb_score_%i", i ) );
		}
	}

	guiOnlineStatus->SetStateInt( "lb_total_rows", callback->GetNumRowsInLeaderboard() );
	guiOnlineStatus->StateChanged( Sys_Milliseconds() );
#endif
}

/*
========================
idGameMainMenuLocal::SetGameComplete
========================
*/
void idGameMainMenuLocal::SetGameComplete()
{
	gameComplete = true;
}

/*
========================
idGameMainMenuLocal::IsShowingIntro
========================
*/
bool idGameMainMenuLocal::IsShowingIntro()
{
	return showingIntro;
}

/*
========================
idGameMainMenuLocal::IsGameComplete
========================
*/
bool idGameMainMenuLocal::IsGameComplete()
{
	return gameComplete;
}

/*
========================
idGameMainMenuLocal::DispatchCommand
========================
*/
void idGameMainMenuLocal::DispatchCommand( idUserInterface* gui, const char* menuCommand )
{
	if( gui == NULL )
	{
		gui = guiActive;
	}

	if( gui == guiMainMenu )
	{
		HandleMainMenuCommands( menuCommand );
		return;
	}
	else if( gui == guiIntro )
	{
		HandleIntroMenuCommands( menuCommand );
	}
	else if( gui == guiRestartMenu )
	{
		HandleRestartMenuCommands( menuCommand );
	}
	else if( guiActive && guiActive->State().GetBool( "gameDraw" ) )
	{
		HandleMainMenuCommands( menuCommand );
	}
	else if( !inGame )
	{
		common->DPrintf( "idGameMainMenuLocal::DispatchCommand: no dispatch found for command '%s'\n", menuCommand );
	}

	if( inGame )
	{
		HandleInGameCommands( menuCommand );
	}
}

/*
========================
idGameMainMenuLocal::HandleSaveGameMenuCommand
========================
*/
bool idGameMainMenuLocal::HandleSaveGameMenuCommand( idCmdArgs& args, int& icmd )
{
	const char* cmd = args.Argv( icmd - 1 );

	if( idStr::Icmp( cmd, "loadGame" ) == 0 )
	{
		char buf[128];
		int choice = ( saveGameListGUI != NULL ) ? saveGameListGUI->GetSelection( buf, sizeof( buf ), 0 ) : -1;
		if( choice >= 0 && choice < sortedSaves.Num() )
		{
			common->LoadGame( sortedSaves[choice].slotName );
		}
		return true;
	}

	if( idStr::Icmp( cmd, "saveGame" ) == 0 )
	{
		const char* saveGameName = guiActive->State().GetString( "saveGameName" );
		if( saveGameName != NULL && saveGameName[0] != '\0' )
		{
			common->SaveGame( saveGameName );
			SetSaveGameGuiVars();
		}
		return true;
	}

	if( idStr::Icmp( cmd, "deleteGame" ) == 0 )
	{
		char buf[128];
		int choice = ( saveGameListGUI != NULL ) ? saveGameListGUI->GetSelection( buf, sizeof( buf ), 0 ) : -1;
		if( choice >= 0 && choice < sortedSaves.Num() )
		{
			session->DeleteSaveGameSync( sortedSaves[choice].slotName );
			SetSaveGameGuiVars();
		}
		return true;
	}

	if( idStr::Icmp( cmd, "updateSaveGameInfo" ) == 0 )
	{
		char buf[128];
		int choice = ( saveGameListGUI != NULL ) ? saveGameListGUI->GetSelection( buf, sizeof( buf ), 0 ) : -1;
		if( choice >= 0 && choice < sortedSaves.Num() )
		{
			const idSaveGameDetails& details = sortedSaves[choice];

			const char* difficultyStr = "";
			switch( details.GetDifficulty() )
			{
				case 0:
					difficultyStr = idLocalization::GetString( "#str_04089" );
					break;
				case 1:
					difficultyStr = idLocalization::GetString( "#str_04091" );
					break;
				case 2:
					difficultyStr = idLocalization::GetString( "#str_04093" );
					break;
				case 3:
					difficultyStr = idLocalization::GetString( "#str_02357" );
					break;
			}

			const char* campaignStr = "";
			switch( details.GetExpansion() )
			{
				case GAME_D3XP:
					campaignStr = idLocalization::GetString( "#str_swf_resurrection" );
					break;
				case GAME_D3LE:
					campaignStr = idLocalization::GetString( "#str_swf_lost_episodes" );
					break;
				case GAME_BASE:
					campaignStr = idLocalization::GetString( "#str_swf_doom3" );
					break;
				default:
					campaignStr = idLocalization::GetString( "#str_savegame_title" );
					break;
			}

			guiActive->SetStateString( "saveGameDifficulty", difficultyStr );
			guiActive->SetStateString( "saveGameCampaign", campaignStr );
			guiActive->SetStateString( "saveGameDate", Sys_TimeStampToStr( details.date ) );
			guiActive->SetStateString( "saveGameTime", Sys_SecToStr( details.GetPlaytime() ) );
			//guiActive->SetStateString( "loadgame_shot", va( "savegames/%s.png", details.slotName.c_str() ) );
			guiActive->SetStateString( "loadgame_shot", "guis/assets/blankLevelShot" ); // FIXME: Re-implement old doom 3 save screenshot system
			guiActive->StateChanged( Sys_Milliseconds() );
		}
		return true;
	}

	return false;
}

/*
========================
idGameMainMenuLocal::HandleMainMenuCommands

Executes any commands returned by the gui
========================
*/
void idGameMainMenuLocal::HandleMainMenuCommands( const char* menuCommand )
{
	idCmdArgs args;
	args.TokenizeString( menuCommand, false );

	for( int icmd = 0; icmd < args.Argc(); )
	{
		const char* cmd = args.Argv( icmd++ );

		if( HandleSaveGameMenuCommand( args, icmd ) )
		{
			continue;
		}

		if( idStr::Icmp( cmd, "play" ) == 0 )
		{
			if( args.Argc() - icmd >= 1 && menuSW != NULL )
			{
				idStr snd = args.Argv( icmd++ );
				int channel = 1;
				if( snd.Length() == 1 )
				{
					channel = atoi( snd );
					snd = args.Argv( icmd++ );
				}
				menuSW->PlayShaderDirectly( snd, channel );

			}
			continue;
		}

		if( idStr::Icmp( cmd, "music" ) == 0 )
		{
			if( args.Argc() - icmd >= 1 && menuSW != NULL )
			{
				idStr snd = args.Argv( icmd++ );
				menuSW->PlayShaderDirectly( snd, 2 );
			}
			continue;
		}

		if( idStr::Icmp( cmd, "startGame" ) == 0 )
		{
			cvarSystem->SetCVarInteger( "g_skill", guiMainMenu->State().GetInt( "skill" ) );
			introMapName = ( icmd < args.Argc() ) ? args.Argv( icmd++ ) : "game/mars_city1";

			// need to do this here to make sure Sys_Milliseconds() is correct or the gui activates with a time that
			// is "however long map load took" time in the past
			if( expansionType == 0 )
			{
				guiIntro = guiIntroD3;
			}
			else if( expansionType == 1 && guiIntroD3XP != NULL )
			{
				guiIntro = guiIntroD3XP;
			}
			else if( expansionType == 2 && guiIntroD3LE != NULL )
			{
				guiIntro = guiIntroD3LE;
			}

			if( guiIntro == NULL )
			{
				idLib::Warning( "Unable to select an intro GUI for expansion %d", expansionType );
				continue;
			}

			SetGUI( guiIntro );
			guiIntro->StateChanged( Sys_Milliseconds(), true );

			// stop playing the game sounds
			soundSystem->SetPlayingSoundWorld( menuSW );
			menuSW->StopAllSounds();

			showingIntro = true;

			continue;
		}

		if( idStr::Icmp( cmd, "switchExpansion" ) == 0 )
		{
			// HACK HACK
			expansionType += 1;
			if( expansionType > 2 )
			{
				expansionType = 0;
			}

			guiMainMenu->SetStateInt( "doommap", expansionType );
			guiMainMenu->StateChanged( Sys_Milliseconds() );

			continue;
		}


		if( idStr::Icmp( cmd, "quit" ) == 0 )
		{
			ExitMenu();
			cmdSystem->AppendCommandText( "quit\n" );
			return;
		}

		if( idStr::Icmp( cmd, "close" ) == 0 )
		{
			// if we aren't in a game, the menu can't be closed
			if( inGame )
			{
				ExitMenu();
			}
			continue;
		}

		if( !idStr::Icmp( cmd, "resetdefaults" ) )
		{
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "exec default.cfg" );
			guiMainMenu->SetKeyBindingNames();
			continue;
		}


		if( !idStr::Icmp( cmd, "bind" ) )
		{
			if( args.Argc() - icmd >= 2 )
			{
				int key = atoi( args.Argv( icmd++ ) );
				idStr bind = args.Argv( icmd++ );
				if( idKeyInput::NumBinds( bind ) >= 2 && !idKeyInput::KeyIsBoundTo( key, bind ) )
				{
					idKeyInput::UnbindBinding( bind );
				}
				idKeyInput::SetBinding( key, bind );
				guiMainMenu->SetKeyBindingNames();
			}
			continue;
		}

		if( idStr::Icmp( cmd, "video" ) == 0 )
		{
			idStr vcmd;
			if( args.Argc() - icmd >= 1 )
			{
				vcmd = args.Argv( icmd++ );
			}

			int oldSpec = com_machineSpec.GetInteger();

			if( idStr::Icmp( vcmd, "settings" ) == 0 )
			{
				// TODO: Autofill the video modes.
				static const int videoModes[][2] =
				{
					{ 640, 480 }, { 800, 600 }, { 1024, 768 },
					{ 1152, 864 }, { 1280, 1024 }, { 1600, 1200 }
				};
				const int modeIndex = idMath::ClampInt( 0, 5, guiActive->State().GetInt( "screenSize" ) );
				static const int fullscreenModes[] = { -1, 0, 1 };
				const int fullscreenIndex = idMath::ClampInt( 0, 2, guiActive->State().GetInt( "fullscreen" ) );
				const int fullscreen = fullscreenModes[ fullscreenIndex ];
				const bool videoChanged = cvarSystem->GetCVarInteger( "r_vidFullscreen" ) != fullscreen ||
										  cvarSystem->GetCVarInteger( "r_vidMode" ) != 0 || cvarSystem->GetCVarInteger( "r_vidWidth" ) != videoModes[ modeIndex ][0] ||
										  cvarSystem->GetCVarInteger( "r_vidHeight" ) != videoModes[ modeIndex ][1];

				cvarSystem->SetCVarInteger( "r_vidFullscreen", fullscreen );
				cvarSystem->SetCVarInteger( "r_vidMode", 0 );
				cvarSystem->SetCVarInteger( "r_vidWidth", videoModes[ modeIndex ][0] );
				cvarSystem->SetCVarInteger( "r_vidHeight", videoModes[ modeIndex ][1] );

				if( videoChanged )
				{
					cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart\n" );
				}
				continue;
			}

			if( idStr::Icmp( vcmd, "low" ) == 0 )
			{
				com_machineSpec.SetInteger( 0 );
			}
			else if( idStr::Icmp( vcmd, "medium" ) == 0 )
			{
				com_machineSpec.SetInteger( 1 );
			}
			else  if( idStr::Icmp( vcmd, "high" ) == 0 )
			{
				com_machineSpec.SetInteger( 2 );
			}
			else  if( idStr::Icmp( vcmd, "ultra" ) == 0 )
			{
				com_machineSpec.SetInteger( 3 );
			}
			else if( idStr::Icmp( vcmd, "recommended" ) == 0 )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "setMachineSpec\n" );
			}

			if( oldSpec != com_machineSpec.GetInteger() )
			{
				guiActive->SetStateInt( "com_machineSpec", com_machineSpec.GetInteger() );
				guiActive->StateChanged( Sys_Milliseconds() );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "execMachineSpec\n" );
			}

			if( idStr::Icmp( vcmd, "restart" )  == 0 )
			{
				guiActive->HandleNamedEvent( "cvar write render" );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart\n" );
			}

			continue;
		}

		if( idStr::Icmp( cmd, "clearBind" ) == 0 )
		{
			if( args.Argc() - icmd >= 1 )
			{
				idKeyInput::UnbindBinding( args.Argv( icmd++ ) );
				guiMainMenu->SetKeyBindingNames();
			}
			continue;
		}

		if( !idStr::Icmp( cmd, "exec" ) )
		{

			//Backup the language so we can restore it after defaults.
			idStr lang = cvarSystem->GetCVarString( "sys_lang" );

			cmdSystem->BufferCommandText( CMD_EXEC_NOW, args.Argv( icmd++ ) );
			if( idStr::Icmp( "cvar_restart", args.Argv( icmd - 1 ) ) == 0 )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "exec default.cfg" );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "setMachineSpec\n" );

				//Make sure that any r_forceAmbient changes take effect
				float bright = cvarSystem->GetCVarFloat( "r_forceAmbient" );
				cvarSystem->SetCVarFloat( "r_forceAmbient", 0.01f );
				cvarSystem->SetCVarFloat( "r_forceAmbient", bright );

				guiActive->SetStateInt( "com_machineSpec", cvarSystem->GetCVarInteger( "com_machineSpec" ) );

				//Restore the language
				cvarSystem->SetCVarString( "sys_lang", lang );

			}
			continue;
		}

		if( !idStr::Icmp( cmd, "loadBinds" ) )
		{
			guiMainMenu->SetKeyBindingNames();
			continue;
		}

		if( !idStr::Icmp( cmd, "systemCvars" ) )
		{
			guiActive->HandleNamedEvent( "cvar read render" );
			guiActive->HandleNamedEvent( "cvar read sound" );
			continue;
		}
	}
}

/*
========================
idGameMainMenuLocal::HandleInGameCommands

Executes any commands returned by the gui
========================
*/
void idGameMainMenuLocal::HandleInGameCommands( const char* menuCommand )
{
	// execute the command from the menu
	idCmdArgs args;

	args.TokenizeString( menuCommand, false );

	const char* cmd = args.Argv( 0 );
	if( !idStr::Icmp( cmd, "close" ) )
	{
		if( guiActive )
		{
			sysEvent_t  ev;
			ev.evType = SE_NONE;
			const char*	cmd;
			cmd = guiActive->HandleEvent( &ev, Sys_Milliseconds() );
			guiActive->Activate( false, Sys_Milliseconds() );
			guiActive = NULL;
		}
	}
}

/*
========================
idGameMainMenuLocal::HandleRestartMenuCommands

Executes any commands returned by the gui
========================
*/
void idGameMainMenuLocal::HandleRestartMenuCommands( const char* menuCommand )
{
	idCmdArgs args;
	args.TokenizeString( menuCommand, false );

	for( int icmd = 0; icmd < args.Argc(); )
	{
		const char* cmd = args.Argv( icmd++ );

		if( HandleSaveGameMenuCommand( args, icmd ) )
		{
			continue;
		}

		if( idStr::Icmp( cmd, "play" ) == 0 )
		{
			if( args.Argc() - icmd >= 1 && menuSW != NULL )
			{
				idStr snd = args.Argv( icmd++ );
				int channel = 1;
				if( snd.Length() == 1 )
				{
					channel = atoi( snd );
					snd = args.Argv( icmd++ );
				}
				menuSW->PlayShaderDirectly( snd, channel );

			}
			continue;
		}

		if( idStr::Icmp( cmd, "music" ) == 0 )
		{
			if( args.Argc() - icmd >= 1 && menuSW != NULL )
			{
				idStr snd = args.Argv( icmd++ );
				menuSW->PlayShaderDirectly( snd, 2 );
			}
			continue;
		}

		if( idStr::Icmp( cmd, "restart" ) == 0 )
		{
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "restartMap" );
			continue;
		}

		if( !idStr::Icmp( cmd, "exec" ) )
		{
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, args.Argv( icmd++ ) );
			continue;
		}

		if( idStr::Icmp( cmd, "quit" ) == 0 )
		{
			ExitMenu();
			cmdSystem->AppendCommandText( "quit\n" );
			return;
		}
	}
}

/*
========================
idGameMainMenuLocal::HandleIntroMenuCommands
========================
*/
void idGameMainMenuLocal::HandleIntroMenuCommands( const char* menuCommand )
{
	idCmdArgs args;
	args.TokenizeString( menuCommand, false );

	for( int i = 0; i < args.Argc(); )
	{
		const char* cmd = args.Argv( i++ );

		if( idStr::Icmp( cmd, "startGame" ) == 0 )
		{
			cmdSystem->AppendCommandText( va( "map %s\n", introMapName.c_str() ) );
			introMapName.Clear();

			showingIntro = false;

			ExitMenu();
			continue;
		}

		if( idStr::Icmp( cmd, "play" ) == 0 )
		{
			if( args.Argc() - i >= 1 && menuSW != NULL )
			{
				menuSW->PlayShaderDirectly( args.Argv( i++ ) );
			}
			continue;
		}
	}
}

/*
========================
idGameMainMenuLocal::GetSaveGameList
========================
*/
void idGameMainMenuLocal::GetSaveGameList()
{
	sortedSaves = session->GetSaveGameManager().GetEnumeratedSavegames();

	for( int a = 1; a < sortedSaves.Num(); a++ )
	{
		idSaveGameDetails key = sortedSaves[a];
		int b = a - 1;
		while( b >= 0 && key < sortedSaves[b] )
		{
			sortedSaves[b + 1] = sortedSaves[b];
			b--;
		}
		sortedSaves[b + 1] = key;
	}
}

/*
========================
idGameMainMenuLocal::SetSaveGameGuiVars
========================
*/
void idGameMainMenuLocal::SetSaveGameGuiVars()
{
	if( guiActive == NULL || saveGameListGUI == NULL )
	{
		return;
	}

	GetSaveGameList();

	saveGameListGUI->Clear();

	if( session->GetSaveGameManager().IsWorking() )
	{
		saveGameListGUI->Push( va( "%s\t\t", idLocalization::GetString( "#str_dlg_refreshing" ) ) );
	}
	else if( sortedSaves.Num() == 0 )
	{
		saveGameListGUI->Push( va( "%s\t\t", idLocalization::GetString( "#str_no_saves_found" ) ) );
	}
	else
	{
		for( int i = 0; i < sortedSaves.Num(); i++ )
		{
			const idSaveGameDetails& details = sortedSaves[i];
			idStr label;

			if( details.damaged )
			{
				label = va( "^1%s", idLocalization::GetString( "#str_swf_corrupt_file" ) );
			}
			else if( details.GetSaveVersion() != SAVEGAME_VERSION )
			{
				label = va( "^1%s", idLocalization::GetString( "#str_swf_wrong_version" ) );
			}
			else
			{
				if( details.slotName.Icmp( "autosave" ) == 0 )
				{
					label = S_COLOR_YELLOW;
				}
				else if( details.slotName.Icmp( "quick" ) == 0 )
				{
					label = S_COLOR_ORANGE;
				}
				label += details.GetMapName();
			}

			label += va( "\t%s\t%s", Sys_TimeStampToStr( details.date ), Sys_SecToStr( details.GetPlaytime() ) );

			saveGameListGUI->Push( label );
		}
	}

	saveGameListGUI->SetSelection( -1 );
	guiActive->SetStateString( "saveGameName", "" );
	guiActive->StateChanged( Sys_Milliseconds() );
}

/*
========================
idGameMainMenuLocal::SetVideoGuiVars
========================
*/
void idGameMainMenuLocal::SetVideoGuiVars()
{
	// TODO: Autofill the video modes.
	static const int videoModes[][2] =
	{
		{ 640, 480 }, { 800, 600 }, { 1024, 768 },
		{ 1152, 864 }, { 1280, 1024 }, { 1600, 1200 }
	};

	int screenSize = 0;

	for( int i = 0; i < 6; i++ )
	{
		if( cvarSystem->GetCVarInteger( "r_vidWidth" ) == videoModes[i][0] && cvarSystem->GetCVarInteger( "r_vidHeight" ) == videoModes[i][1] )
		{
			screenSize = i;
			break;
		}
	}

	auto BuildVideoModeString = []() -> idStr
	{
		idStr result;
		const int numModes = sizeof( videoModes ) / sizeof( videoModes[0] );

		for( int i = 0; i < numModes; i++ )
		{
			if( i > 0 )
			{
				result += ";";
			}
			result += va( "%dx%d", videoModes[i][0], videoModes[i][1] );
		}
		return result;
	};

	idStr modeString = BuildVideoModeString();
	guiMainMenu->SetStateString( "screenSizeChoices", modeString );

	guiMainMenu->SetStateInt( "screenSize", screenSize );
	guiMainMenu->SetStateInt( "fullscreen", idMath::ClampInt( 0, 2, cvarSystem->GetCVarInteger( "r_vidFullscreen" ) + 1 ) );

	guiMainMenu->StateChanged( Sys_Milliseconds(), true );
}

/*
========================
idGameMainMenuLocal::SetMainMenuGuiVars
========================
*/
void idGameMainMenuLocal::SetMainMenuGuiVars()
{
	SetVideoGuiVars();

	guiMainMenu->SetStateString( "serverlist_sel_0", "-1" );
	guiMainMenu->SetStateString( "serverlist_selid_0", "-1" );

	guiMainMenu->SetStateInt( "com_machineSpec", cvarSystem->GetCVarInteger( "com_machineSpec" ) );

	// "inetGame" will hold a hand-typed inet address, which is not archived to a cvar
	guiMainMenu->SetStateString( "inetGame", "" );

	// key bind names
	guiMainMenu->SetKeyBindingNames();

	// flag for in-game menu
	guiMainMenu->SetStateString( "inGame", inGame ? ( common->IsMultiplayer() ? "2" : "1" ) : "0" );

	guiMainMenu->SetStateBool( "canContinue", canContinue );

	//SetCDKeyGuiVars( );
	guiMainMenu->SetStateString( "nightmare", cvarSystem->GetCVarBool( "g_nightmare" ) ? "1" : "0" );
	guiMainMenu->SetStateString( "browser_levelshot", "guis/assets/splash/pdtempa" );

	//SetMainMenuSkin();

	// Mods Menu
	//SetModsMenuGuiVars();

	guiMainMenu->SetStateString( "driver_prompt", "0" );

	//SetPbMenuGuiVars();

	guiMainMenu->SetStateString( "option_status", GameUIUtils::GameUI_EnableDisableStr() );

	guiMainMenu->SetStateInt( "doommap", expansionType );
}
