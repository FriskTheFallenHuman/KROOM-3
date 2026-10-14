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

const static int NUM_MOD_OPTIONS = 10;

/*
========================
idMenuScreen_Shell_Mods::Initialize
========================
*/
void idMenuScreen_Shell_Mods::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuLoad" );

	modInfo = new( TAG_SWF ) idMenuWidget_Shell_ModInfo();
	modInfo->SetSpritePath( GetSpritePath(), "info", "details" );
	modInfo->Initialize( data );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_MOD_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	while( options->GetChildren().Num() < NUM_MOD_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->RegisterEventObserver( modInfo );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	AddChild( options );
	AddChild( modInfo );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "EXTRAS" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	btnLaunch = new( TAG_SWF ) idMenuWidget_Button();
	btnLaunch->Initialize( data );
	btnLaunch->SetLabel( "LOAD MOD" );
	btnLaunch->SetSpritePath( GetSpritePath(), "info", "btnDelete" );
	btnLaunch->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED );
	AddChild( btnLaunch );

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
idMenuScreen_Shell_Mods::RefreshModList
========================
*/
void idMenuScreen_Shell_Mods::RefreshModList()
{
	cachedMods.Clear();

	idModList* mods = fileSystem->ListMods();
	if( mods != NULL )
	{
		for( int i = 0; i < mods->GetNumMods(); ++i )
		{
			cachedMods.Append( mods->GetInfo( i ) );
		}
		fileSystem->FreeModList( mods );
	}

	if( modInfo != NULL )
	{
		modInfo->SetModList( &cachedMods );
	}

	if( options != NULL )
	{
		idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > listData;

		if( cachedMods.Num() == 0 )
		{
			idList< idStr > line;
			line.Append( "No mods found" );
			listData.Append( line );
		}
		else
		{
			for( int i = 0; i < cachedMods.Num(); ++i )
			{
				const idModInfo_t& info = cachedMods[i];

				idList< idStr > line;

				idStr label;
				if( info.name.Length() == 0 )
				{
					label = "DOOM 3: BFG Edition";                // base game
				}
				else if( info.title.Length() )
				{
					label = info.title;
				}
				else
				{
					label = info.name;
				}

				// Mark the currently running mod with a colour code.
				const char* current = cvarSystem->GetCVarString( "fs_playMod" );
				if( current != NULL && current[0] != '\0' && info.name.Icmp( current ) == 0 )
				{
					label = idStr( S_COLOR_YELLOW ) + label;
				}

				line.Append( label );
				listData.Append( line );
			}
		}

		options->SetListData( listData );
		options->Update();
	}

	if( options != NULL && options->GetTotalNumberOfOptions() > 0 && options->GetViewIndex() >= options->GetTotalNumberOfOptions() )
	{
		options->SetViewIndex( options->GetTotalNumberOfOptions() - 1 );
		if( options->GetViewOffset() > options->GetViewIndex() )
		{
			options->SetViewOffset( options->GetViewIndex() );
		}
		options->SetFocusIndex( options->GetViewIndex() - options->GetViewOffset() );
	}
}

/*
========================
idMenuScreen_Shell_Mods::GetSelectedMod
========================
*/
const idModInfo_t* idMenuScreen_Shell_Mods::GetSelectedMod() const
{
	if( options == NULL || cachedMods.Num() == 0 )
	{
		return NULL;
	}

	const int index = options->GetViewIndex();
	if( index < 0 || index >= cachedMods.Num() )
	{
		return NULL;
	}

	return &cachedMods[ index ];
}

/*
========================
idMenuScreen_Shell_Mods::Update
========================
*/
void idMenuScreen_Shell_Mods::Update()
{
	RefreshModList();

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_04098" );
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
idMenuScreen_Shell_Mods::ShowScreen
========================
*/
void idMenuScreen_Shell_Mods::ShowScreen( const mainMenuTransition_t transitionType )
{
	RefreshModList();
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Mods::HideScreen
========================
*/
void idMenuScreen_Shell_Mods::HideScreen( const mainMenuTransition_t transitionType )
{
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Mods::HandleAction
========================
*/
bool idMenuScreen_Shell_Mods::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{
	if( menuData != NULL )
	{
		if( menuData->ActiveScreen() != SHELL_AREA_MODS )
		{
			return false;
		}
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			if( menuData != NULL )
			{
				menuData->SetNextScreen( SHELL_AREA_EXTRAS, MENU_TRANSITION_SIMPLE );
			}
			return true;
		}

		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( options == NULL || cachedMods.Num() == 0 )
			{
				return true;
			}

			if( parms.Num() == 1 )
			{
				const int selectionIndex = parms[0].ToInteger();
				if( selectionIndex != options->GetFocusIndex() )
				{
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
					options->SetFocusIndex( selectionIndex );
					return true;
				}
			}

			const idModInfo_t* chosen = GetSelectedMod();
			if( chosen == NULL )
			{
				return true;
			}

			if( chosen->name.Length() == 0 )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "switchMod\n" );
				return true;
			}

			// launch the select mod
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "switchMod \"%s\"\n", chosen->name.c_str() ) );
			return true;
		}

		case WIDGET_ACTION_SCROLL_VERTICAL:
		{
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

//****************************************************************
// idMenuWidget_Shell_ModInfo
//****************************************************************

/*
========================
idMenuWidget_Shell_ModInfo::Update
========================
*/
void idMenuWidget_Shell_ModInfo::Update()
{
	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( !BindSprite( root ) || GetSprite() == NULL )
	{
		return;
	}

	idSWFTextInstance* infoSprite = GetSprite()->GetScriptObject()->GetNestedText( "txtDesc" );
	if( infoSprite == NULL )
	{
		return;
	}

	infoSprite->SetIgnoreColor( false );

	if( modList == NULL || modIndex < 0 || modIndex >= modList->Num() )
	{
		infoSprite->SetText( "" );
		return;
	}

	const idModInfo_t& info = ( *modList )[ modIndex ];

	if( info.name.Length() == 0 )
	{
		infoSprite->SetText( "DOOM 3: BFG Edition" );
		return;
	}

	const char* typeStr =
		info.type == MODTYPE_SINGLEPLAYER ? idLocalization::GetString( "#str_01861" ) :
		info.type == MODTYPE_MULTIPLAYER  ? idLocalization::GetString( "#str_01862" ) :
		idLocalization::GetString( "Both" );

	idStr body;

	if( info.title.Length() )
	{
		body.Append( info.title );
		body.Append( "\n" );
	}

	if( info.developer.Length() )
	{
		body.Append( va( "%s: %s\n", idLocalization::GetString( "Developter Name" ), info.developer.c_str() ) );
	}
	if( info.url.Length() )
	{
		body.Append( va( "%s: %s\n", idLocalization::GetString( "URL" ), info.url.c_str() ) );
	}
	body.Append( va( "%s: %s\n", idLocalization::GetString( "Type" ), typeStr ) );

	if( info.description.Length() )
	{
		body.Append( "\n" );
		body.Append( info.description );
	}

	if( info.fs_game_base.Length() || info.skipBase )
	{
		body.Append( va( "\n\nfs_game_base: %s\nfs_game: %s\nskipBase: %d", info.fs_game_base.c_str(), info.fs_game.c_str(), info.skipBase ? 1 : 0 ) );
	}

	infoSprite->SetText( body );
}

/*
========================
idMenuWidget_Shell_ModInfo::ObserveEvent
========================
*/
void idMenuWidget_Shell_ModInfo::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{
	if( event.type != WIDGET_EVENT_FOCUS_ON )
	{
		return;
	}

	const idMenuWidget_Button* const button = dynamic_cast< const idMenuWidget_Button* >( &widget );
	if( button == NULL )
	{
		return;
	}

	const idMenuWidget* const listWidget = button->GetParent();
	if( listWidget == NULL )
	{
		return;
	}

	const idMenuWidget_DynamicList* const list = dynamic_cast< const idMenuWidget_DynamicList* const >( listWidget );
	if( list == NULL )
	{
		return;
	}

	modIndex = list->GetViewIndex();
	Update();
}