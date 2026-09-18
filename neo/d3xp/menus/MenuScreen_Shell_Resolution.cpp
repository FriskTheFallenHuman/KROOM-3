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

const static int NUM_VISIBLE_OPTIONS = 8;

/*
========================
idMenuScreen_Shell_Resolution::Initialize
========================
*/
void idMenuScreen_Shell_Resolution::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuResolution" );

	options = new( TAG_SWF ) idMenuWidget_DynamicList();
	options->SetNumVisibleOptions( NUM_VISIBLE_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );

	scrollbar = new( TAG_SWF ) idMenuWidget_ScrollBar();
	scrollbar->SetSpritePath( GetSpritePath(), "info", "scrollbar" );
	scrollbar->RegisterEventObserver( this );
	scrollbar->Initialize( data );

	while( options->GetChildren().Num() < NUM_VISIBLE_OPTIONS )
	{
		idMenuWidget_Button* const buttonWidget = new( TAG_SWF ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->RegisterEventObserver( scrollbar );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );
	options->AddChild( scrollbar );
	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_00183" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( btnBack );

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
idMenuScreen_Shell_Resolution::Update
========================
*/
void idMenuScreen_Shell_Resolution::Update()
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
			heading->SetText( "#str_02154" );
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
idMenuScreen_Shell_Resolution::ShowScreen
========================
*/
void idMenuScreen_Shell_Resolution::ShowScreen( const mainMenuTransition_t transitionType )
{
	originalOption.monitor = cvarSystem->GetCVarInteger( "r_vidMonitor" );
	originalOption.width = cvarSystem->GetCVarInteger( "r_vidWidth" );
	originalOption.height = cvarSystem->GetCVarInteger( "r_vidHeight" );
	originalOption.displayHz = cvarSystem->GetCVarInteger( "r_vidDisplayRefresh" );
	originalFullscreen = cvarSystem->GetCVarInteger( "r_vidFullscreen" );
	originalVidMode = cvarSystem->GetCVarInteger( "r_vidMode" );

	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;
	optionData.Clear();

	int viewIndex = 0;
	idList< idList<vidMode_t> > displays;

	const int numDisplays = ( idDeviceManager::GetInstance() && idDeviceManager::GetInstance()->GetNumVideoDisplays() );
	for( int displayNum = 0; displayNum < numDisplays; ++displayNum )
	{
		idList<vidMode_t>& modeList = displays.Alloc();
		if( !idDeviceManager::GetInstance() || !idDeviceManager::GetInstance()->GetModeListForDisplay( displayNum, modeList, 600 ) ) // get modes with height >= 600 only
		{
			displays.RemoveIndex( displays.Num() - 1 );
			break;
		}
	}
	for( int displayNum = 0; displayNum < displays.Num(); displayNum++ )
	{
		idList<vidMode_t>& modeList = displays[displayNum];

		idList<vidMode_t> uniqueModes;
		for( int i = 0; i < modeList.Num(); i++ )
		{
			const vidMode_t& m = modeList[i];
			bool merged = false;
			for( int j = 0; j < uniqueModes.Num(); j++ )
			{
				if( uniqueModes[j].width == m.width && uniqueModes[j].height == m.height )
				{
					if( m.displayHz > uniqueModes[j].displayHz )
					{
						uniqueModes[j].displayHz = m.displayHz;
					}
					merged = true;
					break;
				}
			}
			if( !merged )
			{
				uniqueModes.Append( m );
			}
		}

		for( int i = 0; i < uniqueModes.Num(); i++ )
		{
			const vidMode_t& m = uniqueModes[i];
			const optionData_t thisOption( displayNum + 1, m.width, m.height, m.displayHz );

			// Mark the currently selected resolution as the focused entry.
			if( originalVidMode == 0 && originalOption == thisOption )
			{
				viewIndex = menuOptions.Num();
			}

			idStr str;
			if( displays.Num() > 1 )
			{
				str.Append( va( "%s %d: ", idLocalization::GetString( "#str_swf_monitor" ), displayNum + 1 ) );
			}
			str.Append( va( "%d x %d", m.width, m.height ) );

			menuOptions.Alloc().Alloc() = str;
			optionData.Append( thisOption );
		}
	}

	options->SetListData( menuOptions );
	options->SetViewIndex( viewIndex );
	const int topOfLastPage = menuOptions.Num() - NUM_VISIBLE_OPTIONS;
	if( viewIndex < NUM_VISIBLE_OPTIONS )
	{
		options->SetViewOffset( 0 );
		options->SetFocusIndex( viewIndex );
	}
	else if( viewIndex >= topOfLastPage )
	{
		options->SetViewOffset( topOfLastPage );
		options->SetFocusIndex( viewIndex - topOfLastPage );
	}
	else
	{
		options->SetViewOffset( viewIndex );
		options->SetFocusIndex( 0 );
	}

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Resolution::HideScreen
========================
*/
void idMenuScreen_Shell_Resolution::HideScreen( const mainMenuTransition_t transitionType )
{
	if( scrollbar && scrollbar->dragging )
	{
		scrollbar->dragging = false;
	}
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Resolution::HandleAction
========================
*/
bool idMenuScreen_Shell_Resolution::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_RESOLUTION )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
		{
			if( options != NULL )
			{
				int selectionIndex = options->GetFocusIndex();
				if( parms.Num() == 1 )
				{
					selectionIndex = parms[0].ToInteger();
				}

				if( options->GetFocusIndex() != selectionIndex )
				{
					options->SetFocusIndex( selectionIndex );
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				}
				const optionData_t& currentOption = optionData[options->GetViewIndex()];

				if( originalVidMode == 0 && currentOption == originalOption )
				{
					// Same resolution as before? nothing to do.
					menuData->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
				}
				else
				{
					// Change the resolution.
					cvarSystem->SetCVarInteger( "r_vidMode", 0 );
					cvarSystem->SetCVarInteger( "r_vidMonitor", currentOption.monitor );
					cvarSystem->SetCVarInteger( "r_vidWidth", currentOption.width );
					cvarSystem->SetCVarInteger( "r_vidHeight", currentOption.height );
					cvarSystem->SetCVarInteger( "r_windowWidth", currentOption.width );
					cvarSystem->SetCVarInteger( "r_windowHeight", currentOption.height );

					// If the current refresh rate is not available at the new
					// resolution, fall back to auto.
					const int curRate = cvarSystem->GetCVarInteger( "r_vidDisplayRefresh" );
					if( curRate != 0 && idDeviceManager::GetInstance() != NULL )
					{
						idList<int> rates;
						bool found = false;
						if( idDeviceManager::GetInstance()->GetRefreshRatesForDisplay( currentOption.monitor - 1, currentOption.width, currentOption.height, rates ) )
						{
							for( int i = 0; i < rates.Num(); i++ )
							{
								if( rates[i] == curRate )
								{
									found = true;
									break;
								}
							}
						}
						if( !found )
						{
							cvarSystem->SetCVarInteger( "r_vidDisplayRefresh", 0 );
						}
					}

					cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );

					if( cvarSystem->GetCVarInteger( "r_vidFullscreen" ) > 0 )
					{
						class idDialogAcceptVideoChanges : public idDialogCallback
						{
						public:
							idDialogAcceptVideoChanges( idMenuHandler* _menu, gameDialogMessages_t _msg, const optionData_t& _optionData, int _fullscreen, int _vidMode, bool _accept )
							{
								menuHandler = _menu;
								msg = _msg;
								optionData = _optionData;
								fullscreen = _fullscreen;
								vidMode = _vidMode;
								accept = _accept;
							}
							void Call() override
							{
								dialogs->ClearDialog( msg );
								if( accept )
								{
									renderSystem->AdjustFramerateFromDisplayHz( optionData.displayHz );
									cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
									if( menuHandler != NULL )
									{
										menuHandler->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
									}
								}
								else
								{
									cvarSystem->SetCVarInteger( "r_vidFullscreen", fullscreen );
									cvarSystem->SetCVarInteger( "r_vidMode", vidMode );
									cvarSystem->SetCVarInteger( "r_vidMonitor", optionData.monitor );
									cvarSystem->SetCVarInteger( "r_vidWidth", optionData.width );
									cvarSystem->SetCVarInteger( "r_vidHeight", optionData.height );
									cvarSystem->SetCVarInteger( "r_vidDisplayRefresh", optionData.displayHz );
									cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
									// this callback is called from the game thread if the revert timer expires, and since
									// vid_restart must be called from the main thread, CMD_EXEC_APPEND "must be" used here
									// with BufferCommandText instead of CMD_EXEC_NOW
									cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
								}
							}
						private:
							idMenuHandler* menuHandler;
							gameDialogMessages_t msg;
							optionData_t optionData;
							int fullscreen;
							int vidMode;
							bool accept;
						};
						ADD_DIALOG( GDM_CONFIRM_VIDEO_CHANGES, DIALOG_TIMER_ACCEPT_REVERT,
									new( TAG_SWF ) idDialogAcceptVideoChanges( menuData, GDM_CONFIRM_VIDEO_CHANGES, currentOption, 1, 0, true ),
									new( TAG_SWF ) idDialogAcceptVideoChanges( menuData, GDM_CONFIRM_VIDEO_CHANGES, originalOption, originalFullscreen, originalVidMode, false ), true );
					}
					else
					{
						renderSystem->AdjustFramerateFromDisplayHz( currentOption.displayHz );
						menuData->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
					}
				}
				return true;
			}
			return true;
		}
		case WIDGET_ACTION_SCROLL_VERTICAL_VARIABLE:
		{
			if( parms.Num() == 0 || options == NULL || ( scrollbar && scrollbar->dragging ) )
			{
				return true;
			}
			int scroll = parms[0].ToInteger();
			options->Scroll( scroll, true );
			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuScreen_Shell_Resolution::ObserveEvent
========================
*/
void idMenuScreen_Shell_Resolution::ObserveEvent( const idMenuWidget& widget, const idWidgetEvent& event )
{
	const idMenuWidget_ScrollBar* eventScrollbar = dynamic_cast<const idMenuWidget_ScrollBar*>( &widget );
	if( scrollbar != NULL && scrollbar == eventScrollbar && event.type == WIDGET_EVENT_DRAG_START )
	{
		return;
	}

	idMenuScreen::ObserveEvent( widget, event );
}