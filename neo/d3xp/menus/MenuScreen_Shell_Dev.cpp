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
#include "precompiled.h"
#pragma hdrstop
#include "../Game_local.h"

const static int NUM_DEV_OPTIONS = 8;


/*
========================
MapDefIsMultiplayer

Returns true if the mapDef has at least one MP game mode flag set.
Mirrors the logic in idCommonLocal::InitializeMPMapsModes().
========================
*/
static bool MapDefIsMultiplayer( const idDeclEntityDef* mapDef )
{
	if( mapDef == NULL )
	{
		return false;
	}

	const char* gameModes[] =
	{
		"Deathmatch", "Tourney", "Team DM", "Last Man", "CTF", NULL
	};

	for( int i = 0; gameModes[i] != NULL; i++ )
	{
		if( mapDef->dict.GetBool( gameModes[i], false ) )
		{
			return true;
		}
	}
	return false;
}

/*
========================
idMenuScreen_Shell_Dev::Initialize
========================
*/
void idMenuScreen_Shell_Dev::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuSettings" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_DEV_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );

	while( options->GetChildren().Num() < NUM_DEV_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_campaign" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( btnBack );

	SetupDevOptions();
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new( TAG_SWF ) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
}

/*
========================
idMenuScreen_Shell_Dev::SetupDevOptions
========================
*/
void idMenuScreen_Shell_Dev::SetupDevOptions()
{
	devOptions.Clear();

	idHashIndex seenMaps;
	idStrList   seenMapNames;

	auto TryAddMap = [&]( const idStr & mapName, const idStr & displayName, bool isMP )
	{
		if( mapName.IsEmpty() )
		{
			return;
		}
		if( mapName.EndsWithIgnoreCase( "_extra_ents" ) )
		{
			return;
		}

		const int key = seenMaps.GenerateKey( mapName, false );
		for( int idx = seenMaps.GetFirst( key ); idx != idHashIndex::NULL_INDEX; idx = seenMaps.GetNext( idx ) )
		{
			if( idStr::Icmp( seenMapNames[idx], mapName ) == 0 )
			{
				return;
			}
		}

		seenMaps.Add( key, seenMapNames.Num() );
		seenMapNames.Append( mapName );
		devOptions.Append( devOption_t( mapName.c_str(), displayName.c_str(), isMP ) );
	};

	int numMaps = declManager->GetNumDecls( DECL_MAPDEF );
	for( int i = 0; i < numMaps; i++ )
	{
		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(
											declManager->DeclByIndex( DECL_MAPDEF, i ) );
		if( mapDef == NULL )
		{
			continue;
		}

		idStr mapName    = mapDef->GetName();
		idStr displayName = idLocalization::GetString(
								mapDef->dict.GetString( "name", mapName ) );
		bool  isMP       = MapDefIsMultiplayer( mapDef );

		TryAddMap( mapName, displayName, isMP );
	}

	idFileList* files = fileSystem->ListFilesTree( "maps", ".map", true );
	if( files != NULL )
	{
		for( int i = 0; i < files->GetNumFiles(); ++i )
		{
			idStr fullPath = files->GetFile( i );
			idStr shortName = fullPath;
			shortName.StripFileExtension();
			shortName.StripLeading( "maps/" );
			if( shortName.Length() > 0 && shortName[0] == '/' )
			{
				shortName.StripLeading( "/" );
			}

			idStr displayName = shortName;
			bool  isMP        = false;

			const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(
												declManager->FindType( DECL_MAPDEF, shortName, false ) );
			if( mapDef != NULL )
			{
				displayName = idLocalization::GetString(
								  mapDef->dict.GetString( "name", shortName ) );
				isMP = MapDefIsMultiplayer( mapDef );
			}
			else
			{
				idStr alt = shortName;
				alt.StripLeading( "game/" );
				const idDeclEntityDef* mapDef2 = static_cast<const idDeclEntityDef*>(
													 declManager->FindType( DECL_MAPDEF, alt, false ) );
				if( mapDef2 != NULL )
				{
					displayName = idLocalization::GetString(
									  mapDef2->dict.GetString( "name", alt ) );
					isMP = MapDefIsMultiplayer( mapDef2 );
				}
			}

			TryAddMap( shortName, displayName, isMP );
		}
		fileSystem->FreeFileList( files );
	}

	// Build the widget list from the merged, deduplicated devOptions
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
	for( int i = 0; i < devOptions.Num(); ++i )
	{
		idList< idStr > option;
		option.Append( devOptions[i].name );
		menuOptions.Append( option );
	}

	options->SetListData( menuOptions );
}

/*
========================
idMenuScreen_Shell_Dev::Update
========================
*/
void idMenuScreen_Shell_Dev::Update()
{

	if( menuData != NULL )
	{
		idMenuWidget_CommandBar* cmdBar = menuData->GetCmdBar();
		if( cmdBar != NULL )
		{
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t* buttonInfo;
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_00395";
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			if( menuData->GetPlatform() != 2 )
			{
				buttonInfo->label = "#str_SWF_SELECT";
			}
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "DEV" );
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance* gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if( gradient != NULL && heading != NULL )
		{
			gradient->SetXPos( heading->GetTextLength() );
		}
	}

	if( btnBack != NULL )
	{
		btnBack->BindSprite( root );
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_Dev::ShowScreen
========================
*/
void idMenuScreen_Shell_Dev::ShowScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Dev::HideScreen
========================
*/
void idMenuScreen_Shell_Dev::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Dev::HandleAction h
========================
*/
bool idMenuScreen_Shell_Dev::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_DEV )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_CAMPAIGN, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( options == NULL )
			{
				return true;
			}

			int selectionIndex = options->GetViewIndex();
			if( parms.Num() == 1 )
			{
				selectionIndex = parms[0].ToInteger();
			}

			if( options->GetFocusIndex() != selectionIndex - options->GetViewOffset() )
			{
				options->SetFocusIndex( selectionIndex );
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
			}

			int mapIndex = options->GetViewIndex();
			if( ( mapIndex < devOptions.Num() ) && ( !devOptions[ mapIndex ].map.IsEmpty() ) )
			{
				const devOption_t& opt = devOptions[ mapIndex ];
				if( opt.isMultiplayer )
				{
					cmdSystem->AppendCommandText( va( "netmap %s\n", opt.map.c_str() ) );
				}
				else
				{
					if( developer.GetBool() )
					{
						cmdSystem->AppendCommandText( va( "devmap %s\n", opt.map.c_str() ) );
					}
					else
					{
						cmdSystem->AppendCommandText( va( "map %s\n", opt.map.c_str() ) );
					}
				}
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}
