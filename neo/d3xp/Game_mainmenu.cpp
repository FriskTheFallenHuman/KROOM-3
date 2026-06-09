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

// the rest of the engine will only reference the "mainMenu" variable, while all local aspects stay hidden
idGameMainMenuLocal			mainMenuLocal;
idGameMainMenu* 			mainMenu = &mainMenuLocal;	// statically pointed at an idGameMainMenuLocal

/*
========================
idGameMainMenuLocal::idGameMainMenuLocal
========================
*/
idGameMainMenuLocal::idGameMainMenuLocal()
{
	shellHandler = NULL;
	loadGUI = NULL;
	nextLoadTip = 0;
	isHellMap = false;
	defaultLoadscreen = false;
}

/*
========================
idGameMainMenuLocal::Initialize
========================
*/
void idGameMainMenuLocal::Initialize()
{
	shellHandler = new( TAG_SWF ) idMenuHandler_Shell();
}

/*
========================
idGameMainMenuLocal::Shutdown
========================
*/
void idGameMainMenuLocal::Shutdown()
{
	Cleanup();
}

/*
========================
idGameMainMenuLocal::ClearRepeater
========================
*/
void idGameMainMenuLocal::ClearRepeater()
{
	if( shellHandler != NULL )
	{
		shellHandler->ClearWidgetActionRepeater();
	}
}

/*
========================
idGameMainMenuLocal::Init
========================
*/
void idGameMainMenuLocal::Init( const char* filename, idSoundWorld* sw )
{
	if( shellHandler != NULL )
	{
		shellHandler->Initialize( filename, sw );
	}
}

/*
========================
idGameMainMenuLocal::InitMenu
========================
*/
void idGameMainMenuLocal::InitMenu()
{
	// note which media we are going to need to load
	declManager->BeginLevelLoad();
	renderSystem->BeginLevelLoad();
	soundSystem->BeginLevelLoad();
	uiManager->BeginLevelLoad();

	// create main inside an "empty" game level load - so assets get
	// purged automagically when we transition to a "real" map
	CreateMenu( false );
	Show( true );
	SyncWithSession();

	// load
	renderSystem->EndLevelLoad();
	soundSystem->EndLevelLoad();
	declManager->EndLevelLoad();
	uiManager->EndLevelLoad( "" );
}

/*
========================
idGameMainMenuLocal::Cleanup
========================
*/
void idGameMainMenuLocal::Cleanup( bool onlyLoading )
{
	if( !onlyLoading )
	{
		printf( "delete loadGUI;\n" );
	}
	delete loadGUI;
	loadGUI = NULL;

	if( !onlyLoading )
	{
		if( shellHandler != NULL )
		{
			printf( "delete shellHandler;\n" );
			delete shellHandler;
			shellHandler = NULL;
		}

		printf( "mpGame.CleanupScoreboard();\n" );
		gameLocal.mpGame.CleanupScoreboard();
	}
}

/*
===============
idGameMainMenuLocal::LoadLoadingGui
===============
*/
void idGameMainMenuLocal::LoadingGui( const char* mapName, bool& hellMap )
{
	defaultLoadscreen = false;
	loadGUI = new idSWF( "loading/default", NULL );

	extern idCVar g_demoMode;
	if( g_demoMode.GetBool() )
	{
		hellMap = false;
		if( loadGUI != NULL )
		{
			const idMaterial* defaultMat = declManager->FindMaterial( "guis/assets/loadscreens/default" );
			renderSystem->LoadLevelImages();

			loadGUI->Activate( true );
			idSWFSpriteInstance* bgImg = loadGUI->GetRootObject().GetSprite( "bgImage" );
			if( bgImg != NULL )
			{
				bgImg->SetMaterial( defaultMat );
			}
		}
		defaultLoadscreen = true;
		return;
	}

	// load / program a gui to stay up on the screen while loading
	idStrStatic< MAX_OSPATH > stripped = mapName;
	stripped.StripFileExtension();
	stripped.StripPath();

	// use default load screen for demo
	idStrStatic< MAX_OSPATH > matName = "guis/assets/loadscreens/";
	matName.Append( stripped );
	const idMaterial* mat = declManager->FindMaterial( matName );

	renderSystem->LoadLevelImages();

	if( mat->GetImageWidth() < 32 )
	{
		mat = declManager->FindMaterial( "guis/assets/loadscreens/default" );
		renderSystem->LoadLevelImages();
	}

	loadTipList.SetNum( loadTipList.Max() );
	for( int i = 0; i < loadTipList.Max(); ++i )
	{
		loadTipList[i] = i;
	}

	if( loadGUI != NULL )
	{
		loadGUI->Activate( true );
		nextLoadTip = Sys_Milliseconds() + LOAD_TIP_CHANGE_INTERVAL;

		idSWFSpriteInstance* bgImg = loadGUI->GetRootObject().GetSprite( "bgImage" );
		if( bgImg != NULL )
		{
			bgImg->SetMaterial( mat );
		}

		idSWFSpriteInstance* overlay = loadGUI->GetRootObject().GetSprite( "overlay" );

		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_MAPDEF, mapName, false ) );
		if( mapDef != NULL )
		{
			isHellMap = mapDef->dict.GetBool( "hellMap", false );

			if( isHellMap && overlay != NULL )
			{
				overlay->SetVisible( false );
			}

			idStr desc;
			idStr subTitle;
			idStr displayName;
			idSWFTextInstance* txtVal = NULL;

			txtVal = loadGUI->GetRootObject().GetNestedText( "txtRegLoad" );
			displayName = idLocalization::GetString( mapDef->dict.GetString( "name", mapName ) );

			if( txtVal != NULL )
			{
				txtVal->SetText( "#str_00408" );
				txtVal->SetStrokeInfo( true, 2.0f, 1.0f );
			}

			const idMatchParameters& matchParameters = session->GetActingGameStateLobbyBase().GetMatchParms();
			if( matchParameters.gameMode == GAME_MODE_SINGLEPLAYER )
			{
				desc = idLocalization::GetString( mapDef->dict.GetString( "desc", "" ) );
				subTitle = idLocalization::GetString( mapDef->dict.GetString( "subTitle", "" ) );
			}
			else
			{
				const idStrList& modes = common->GetModeDisplayList();
				subTitle = modes[ idMath::ClampInt( 0, modes.Num() - 1, matchParameters.gameMode ) ];

				const char* modeDescs[] = { "#str_swf_deathmatch_desc", "#str_swf_tourney_desc", "#str_swf_team_deathmatch_desc", "#str_swf_lastman_desc", "#str_swf_ctf_desc" };
				desc = idLocalization::GetString( modeDescs[matchParameters.gameMode] );
			}

			if( !isHellMap )
			{
				txtVal = loadGUI->GetRootObject().GetNestedText( "txtName" );
			}
			else
			{
				txtVal = loadGUI->GetRootObject().GetNestedText( "txtHellName" );
			}
			if( txtVal != NULL )
			{
				txtVal->SetText( displayName );
				txtVal->SetStrokeInfo( true, 2.0f, 1.0f );
			}

			txtVal = loadGUI->GetRootObject().GetNestedText( "txtSub" );
			if( txtVal != NULL && !isHellMap )
			{
				txtVal->SetText( subTitle );
				txtVal->SetStrokeInfo( true, 1.75f, 0.75f );
			}

			txtVal = loadGUI->GetRootObject().GetNestedText( "txtDesc" );
			if( txtVal != NULL )
			{
				if( isHellMap )
				{
					txtVal->SetText( va( "\n%s", desc.c_str() ) );
				}
				else
				{
					txtVal->SetText( desc );
				}
				txtVal->SetStrokeInfo( true, 1.75f, 0.75f );
			}
		}
	}
}

/*
========================
idGameMainMenuLocal::IsActive
========================
*/
bool idGameMainMenuLocal::IsLoadingActive() const
{
	if( loadGUI != NULL )
	{
		return loadGUI->IsActive();
	}
	return false;
}

/*
========================
idGameMainMenuLocal::Render
========================
*/
void idGameMainMenuLocal::RenderLoadingShell()
{
	if( loadGUI != NULL )
	{
		loadGUI->Render( renderSystem, Sys_Milliseconds() );
	}
}


/*
========================
idGameMainMenuLocal::CreateMenu
========================
*/
void idGameMainMenuLocal::CreateMenu( bool inGame )
{
	ResetMenu();

	if( shellHandler != NULL )
	{
		if( !inGame )
		{
			shellHandler->SetInGame( false );
			Init( "shell", common->MenuSW() );
		}
		else
		{
			shellHandler->SetInGame( true );
			if( common->IsMultiplayer() )
			{
				Init( "pause", common->SW() );
			}
			else
			{
				Init( "pause", common->MenuSW() );
			}
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
	if( shellHandler != NULL )
	{

		if( !common->IsMultiplayer() && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->health <= 0 )
		{
			return;
		}

		if( shellHandler->GetGameComplete() )
		{
			return;
		}

		shellHandler->SetNextScreen( SHELL_AREA_INVALID, MENU_TRANSITION_SIMPLE );
	}
}

/*
========================
idGameMainMenuLocal::Show
========================
*/
void idGameMainMenuLocal::Show( bool show )
{
	if( shellHandler != NULL )
	{
		shellHandler->ActivateMenu( show );
	}
}

/*
========================
idGameMainMenuLocal::IsActive
========================
*/
bool idGameMainMenuLocal::IsActive() const
{
	if( shellHandler != NULL )
	{
		return shellHandler->IsActive();
	}
	return false;
}

/*
========================
idGameMainMenuLocal::HandleGuiEvent
========================
*/
bool idGameMainMenuLocal::HandleGuiEvent( const sysEvent_t* sev )
{
	if( shellHandler != NULL )
	{
		return shellHandler->HandleGuiEvent( sev );
	}
	return false;
}

/*
========================
idGameMainMenuLocal::Render
========================
*/
void idGameMainMenuLocal::Render()
{
	if( shellHandler != NULL )
	{
		shellHandler->Update();
	}
}

/*
========================
idGameMainMenuLocal::ResetMenu
========================
*/
void idGameMainMenuLocal::ResetMenu()
{
	if( shellHandler != NULL )
	{
		delete shellHandler;
		shellHandler = new( TAG_SWF ) idMenuHandler_Shell();
	}
}

/*
=================
idGameMainMenuLocal::SyncWithSession
=================
*/
void idGameMainMenuLocal::SyncWithSession()
{
	// Update the Loading tip
	const int time = Sys_Milliseconds();
	if( time >= nextLoadTip && loadGUI != NULL && loadTipList.Num() > 0 && !defaultLoadscreen )
	{
		nextLoadTip = time + LOAD_TIP_CHANGE_INTERVAL;
		const int rnd = time % loadTipList.Num();
		idStrStatic<20> tipId;
		tipId.Format( "#str_loadtip_%d", loadTipList[ rnd ] );
		loadTipList.RemoveIndex( rnd );

		idSWFTextInstance* txtVal = loadGUI->GetRootObject().GetNestedText( "txtDesc" );
		if( txtVal != NULL )
		{
			if( isHellMap )
			{
				txtVal->SetText( va( "\n%s", idLocalization::GetString( tipId ) ) );
			}
			else
			{
				txtVal->SetText( idLocalization::GetString( tipId ) );
			}
			txtVal->SetStrokeInfo( true, 1.75f, 0.75f );
		}

		common->UpdateScreen( false );
	}

	// Don't if we dont have a shell
	if( shellHandler == NULL )
	{
		return;
	}

	// Synch with idSession
	switch( session->GetState() )
	{
		case idSession::INGAME:
			shellHandler->SetShellState( SHELL_STATE_PAUSED );
			break;
		case idSession::IDLE:
			shellHandler->SetShellState( SHELL_STATE_IDLE );
			break;
		case idSession::PARTY_LOBBY:
			shellHandler->SetShellState( SHELL_STATE_PARTY_LOBBY );
			break;
		case idSession::GAME_LOBBY:
			shellHandler->SetShellState( SHELL_STATE_GAME_LOBBY );
			break;
		case idSession::SEARCHING:
			shellHandler->SetShellState( SHELL_STATE_SEARCHING );
			break;
		case idSession::LOADING:
			shellHandler->SetShellState( SHELL_STATE_LOADING );
			break;
		case idSession::CONNECTING:
			shellHandler->SetShellState( SHELL_STATE_CONNECTING );
			break;
		case idSession::BUSY:
			shellHandler->SetShellState( SHELL_STATE_BUSY );
			break;
	}
}

/*
========================
idGameMainMenuLocal::SetGameComplete
========================
*/
void idGameMainMenuLocal::SetGameComplete()
{
	if( shellHandler != NULL )
	{
		shellHandler->SetGameComplete();
	}
}

/*
========================
idGameMainMenuLocal::IsShowingIntro
========================
*/
bool idGameMainMenuLocal::IsShowingIntro()
{
	if( !shellHandler )
	{
		return false;
	}
	return shellHandler->IsShowingIntro();
}

/*
========================
idGameMainMenuLocal::IsGameComplete
========================
*/
bool idGameMainMenuLocal::IsGameComplete()
{
	if( !shellHandler )
	{
		return false;
	}
	return shellHandler->GetGameComplete();
}

/*
========================
idGameMainMenuLocal::GameLobby
========================
*/
void idGameMainMenuLocal::UpdateSavedGames()
{
	if( shellHandler != NULL )
	{
		shellHandler->UpdateSavedGames();
	}
}

/*
========================
idGameMainMenuLocal::SetCanContinue
========================
*/
void idGameMainMenuLocal::SetCanContinue( bool valid )
{
	if( shellHandler != NULL )
	{
		shellHandler->SetCanContinue( valid );
	}
}

/*
========================
idGameMainMenuLocal::SetState_GameLobby
========================
*/
void idGameMainMenuLocal::UpdateClientCountdown( int countdown )
{
	if( shellHandler != NULL )
	{
		shellHandler->SetTimeRemaining( countdown );
	}
}

/*
========================
idGameMainMenuLocal::SetState_GameLobby
========================
*/
void idGameMainMenuLocal::UpdateLeaderboard( const idLeaderboardCallback* callback )
{
	if( shellHandler != NULL )
	{
		shellHandler->UpdateLeaderboard( callback );
	}
}