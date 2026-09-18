/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans

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

const static int NUM_SYSTEM_OPTIONS_OPTIONS = 8;

/*
========================
idMenuScreen_Shell_SystemOptions::Initialize
========================
*/
void idMenuScreen_Shell_SystemOptions::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuSystemOptions" );

	options = new( TAG_SWF ) idMenuWidget_SystemOptionsList(); // RB: allow more options than defined in the SWF
	options->SetNumVisibleOptions( NUM_SYSTEM_OPTIONS_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	options->SetControlList( true );
	options->Initialize( data );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_settings" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( options );
	AddChild( btnBack );

	idMenuWidget_ControlButton* control;

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_02154" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_swf_resolution" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_RESOLUTION );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_RESOLUTION );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Refresh Rate" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_REFRESH_RATE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_REFRESH_RATE );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Frame Rate (Integer)" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_INT );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_INT );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Frame Rate (Fraction)" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_FRAC );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, options->GetChildren().Num() );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "High Resolution Clock" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_HIGH_RESOLUTION_CLOCK );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, options->GetChildren().Num() );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_04126" );
	control->SetDataSource( &systemData, idMenuDataSource_SystemSettings::SYSTEM_FIELD_VSYNC );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_SystemSettings::SYSTEM_FIELD_VSYNC );
	options->AddChild( control );

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
idMenuScreen_Shell_SystemOptions::Update
========================
*/
void idMenuScreen_Shell_SystemOptions::Update()
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
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();
	if( BindSprite( root ) )
	{
		idSWFTextInstance* heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if( heading != NULL )
		{
			heading->SetText( "#str_00183" );	// FULLSCREEN
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
idMenuScreen_Shell_SystemOptions::ShowScreen
========================
*/
void idMenuScreen_Shell_SystemOptions::ShowScreen( const mainMenuTransition_t transitionType )
{
	systemData.LoadData();

	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_SystemOptions::HideScreen
========================
*/
void idMenuScreen_Shell_SystemOptions::HideScreen( const mainMenuTransition_t transitionType )
{

	bool skipRestart = false;
	if( menuData && menuData->NextScreen() == SHELL_AREA_RESOLUTION )
	{
		systemData.SetWentToFullscreenMenu( true );
		skipRestart = true;
	}

	if( !skipRestart && systemData.IsRestartRequired() )
	{
		class idDialogRestartCallback : public idDialogCallback
		{
		public:
			idDialogRestartCallback( gameDialogMessages_t _msg, bool _restart )
			{
				msg = _msg;
				restart = _restart;
			}
			void Call() override
			{
				dialogs->ClearDialog( msg );
				if( restart )
				{
					idStr cmdLine = sys->GetCmdLine();
					if( cmdLine.Find( "com_skipIntroVideos" ) < 0 )
					{
						cmdLine.Append( " +set com_skipIntroVideos 1" );
					}
					if( cmdLine.Find( "com_skipLegalScreens" ) < 0 )
					{
						cmdLine.Append( " +set com_skipLegalScreens 1" );
					}
					sys->ReLaunch( ( void* )cmdLine.c_str() );
				}
			}
		private:
			gameDialogMessages_t msg;
			bool restart;
		};
		idStaticList<idDialogCallback*, 4> callbacks;
		idStaticList<idStrId, 4> optionText;
		callbacks.Append( new idDialogRestartCallback( GDM_GAME_RESTART_REQUIRED, false ) );
		callbacks.Append( new idDialogRestartCallback( GDM_GAME_RESTART_REQUIRED, true ) );
		optionText.Append( idStrId( "#str_00100113" ) ); // Continue
		optionText.Append( idStrId( "#str_02487" ) ); // Restart Now
		ADD_DYNAMIC_DIALOG( GDM_GAME_RESTART_REQUIRED, callbacks, optionText, true, idStr() );
	}

	if( systemData.IsDataChanged() )
	{
		systemData.CommitData();
	}

	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_SystemOptions::HandleAction h
========================
*/
bool idMenuScreen_Shell_SystemOptions::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_SYSTEM_OPTIONS )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	if( actionType == WIDGET_ACTION_GO_BACK )
	{
		if( menuData != NULL )
		{
			menuData->SetNextScreen( SHELL_AREA_SETTINGS, MENU_TRANSITION_SIMPLE );
		}
		return true;
	}
	else if( actionType == WIDGET_ACTION_ADJUST_FIELD )
	{
		if( widget )
		{
			if( widget->GetDataSourceFieldIndex() == idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN ||
					widget->GetDataSourceFieldIndex() == idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_INT ||
					widget->GetDataSourceFieldIndex() == idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_FRAC )
			{
				if( widget->GetDataSource() == NULL || widget->GetParent() == NULL || parms.Num() < 1 )
				{
					return true;
				}
				widget->GetDataSource()->AdjustField( widget->GetDataSourceFieldIndex(), parms[0].ToInteger() );
				widget->GetParent()->GetChildByIndex( idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN ).Update();
				widget->GetParent()->GetChildByIndex( idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_INT ).Update();
				widget->GetParent()->GetChildByIndex( idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_FRAC ).Update();
				return true;
			}
		}
	}
	else if( actionType == WIDGET_ACTION_COMMAND )
	{
		if( options == NULL )
		{
			return true;
		}
		int selectionIndex = options->GetFocusIndex();
		if( parms.Num() > 0 )
		{
			selectionIndex = parms[0].ToInteger();
		}
		else if( selectionIndex == idMenuDataSource_SystemSettings::SYSTEM_FIELD_RESOLUTION )
		{
			menuData->SetNextScreen( SHELL_AREA_RESOLUTION, MENU_TRANSITION_SIMPLE );
		}
		else
		{
			systemData.AdjustField( selectionIndex, 1 );
			options->Update();
		}
	}
	else if( actionType == WIDGET_ACTION_START_REPEATER )
	{

		if( options == NULL )
		{
			return true;
		}

		if( parms.Num() == 4 )
		{
			int selectionIndex = parms[3].ToInteger();
			if( selectionIndex != options->GetFocusIndex() )
			{
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				options->SetFocusIndex( selectionIndex );
			}
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/////////////////////////////////
// SCREEN SETTINGS
/////////////////////////////////

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::idMenuDataSource_SystemSettings
========================
*/
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::idMenuDataSource_SystemSettings()
{
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::LoadData
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::LoadData()
{
	if( !wentToFullscreenMenu )
	{
		originalFramerate = cvarSystem->GetCVarFloat( "com_engineHz" );
	}
	else
	{
		wentToFullscreenMenu = false;
	}
	originalHighResolutionClock = cvarSystem->GetCVarInteger( "com_hiResClock" );
	originalVsync = cvarSystem->GetCVarInteger( "r_swapInterval" );

	// Rebuild the refresh-rate list for the current resolution.
	refreshRates.Clear();
	refreshRates.Append( 0 );

	if( idDeviceManager::GetInstance() != NULL )
	{
		int displayIdx = cvarSystem->GetCVarInteger( "r_vidMonitor" ) - 1;
		if( displayIdx < 0 )
		{
			displayIdx = 0;
		}

		int width = cvarSystem->GetCVarInteger( "r_vidWidth" );
		int height = cvarSystem->GetCVarInteger( "r_vidHeight" );
		if( cvarSystem->GetCVarInteger( "r_vidFullscreen" ) == 0 )
		{
			// In windows mode the effective resolution is the window size.
			width = cvarSystem->GetCVarInteger( "r_windowWidth" );
			height = cvarSystem->GetCVarInteger( "r_windowHeight" );
		}

		idList<int> rates;
		if( idDeviceManager::GetInstance()->GetRefreshRatesForDisplay( displayIdx, width, height, rates ) )
		{
			for( int i = 0; i < rates.Num(); i++ )
			{
				refreshRates.Append( rates[i] );
			}
		}
	}

	// If the currently selected rate is not available at this resolution,
	// fall back to Auto.
	const int curRate = cvarSystem->GetCVarInteger( "r_vidDisplayRefresh" );
	if( curRate != 0 )
	{
		bool found = false;
		for( int i = 0; i < refreshRates.Num(); i++ )
		{
			if( refreshRates[i] == curRate )
			{
				found = true;
				break;
			}
		}
		if( !found )
		{
			cvarSystem->SetCVarInteger( "r_vidDisplayRefresh", 0 );
		}
	}
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsRestartRequired
========================
*/
bool idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsRestartRequired() const
{
	if( originalFramerate != cvarSystem->GetCVarFloat( "com_engineHz" ) )
	{
		return true;
	}
	return false;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::CommitData
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::CommitData()
{
	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );
}

/*
========================
AdjustOption
Given a current value in an array of possible values, returns the next n value
========================
*/
int AdjustOption( int currentValue, const int values[], int numValues, int adjustment )
{
	int index = 0;
	for( int i = 0; i < numValues; i++ )
	{
		if( currentValue == values[i] )
		{
			index = i;
			break;
		}
	}
	index += adjustment;
	while( index < 0 )
	{
		index += numValues;
	}
	index %= numValues;
	return values[index];
}

/*
========================
LinearAdjust
Linearly converts a float from one scale to another
========================
*/
float LinearAdjust( float input, float currentMin, float currentMax, float desiredMin, float desiredMax )
{
	return ( ( input - currentMin ) / ( currentMax - currentMin ) ) * ( desiredMax - desiredMin ) + desiredMin;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::AdjustField
========================
*/
void idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{
	if( fieldIndex < 0 || fieldIndex >= MAX_SYSTEM_FIELDS )
	{
		return;
	}
	switch( fieldIndex )
	{
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN:
		{
			static const int numValues = 3;
			static const int values[numValues] = { -1, 0, 1 };

			int current = cvarSystem->GetCVarInteger( "r_vidFullscreen" );
			if( current == -2 )
			{
				current = 1;
			}
			const int next = AdjustOption( current, values, numValues, adjustAmount );

			if( next != current )
			{
				cvarSystem->SetCVarInteger( "r_vidFullscreen", next );

				static int lastVidRestartQueueTime = 0;
				const int now = ( int )sys->GetMilliseconds();
				if( now - lastVidRestartQueueTime > 500 )
				{
					lastVidRestartQueueTime = now;
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
				}
			}
			break;
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_REFRESH_RATE:
		{
			if( refreshRates.Num() == 0 )
			{
				break;
			}

			int currentRate = cvarSystem->GetCVarInteger( "r_vidDisplayRefresh" );
			int idx = 0;
			for( int i = 0; i < refreshRates.Num(); i++ )
			{
				if( refreshRates[i] == currentRate )
				{
					idx = i;
					break;
				}
			}

			idx += adjustAmount;
			while( idx < 0 )
			{
				idx += refreshRates.Num();
			}
			idx %= refreshRates.Num();

			const int newRate = refreshRates[idx];
			if( newRate != currentRate )
			{
				cvarSystem->SetCVarInteger( "r_vidDisplayRefresh", newRate );

				static int lastRefreshRestartTime = 0;
				const int now = ( int )sys->GetMilliseconds();
				if( now - lastRefreshRestartTime > 500 )
				{
					lastRefreshRestartTime = now;
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
				}
			}
			break;
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_INT:
		{
			float engineHz = cvarSystem->GetCVarFloat( "com_engineHz" );
			engineHz += adjustAmount;
			engineHz = idMath::Rint( engineHz * 100.0f ) / 100.0f;
			if( idMath::Frac( engineHz ) == 0.0f )
			{
				cvarSystem->SetCVarFloat( "com_engineHz", engineHz );
			}
			else
			{
				cvarSystem->SetCVarString( "com_engineHz", va( "%.2f", engineHz ) );
			}
			break;
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_FRAC:
		{
			float engineHz = cvarSystem->GetCVarFloat( "com_engineHz" );
			engineHz = idMath::Rint( engineHz * 100.0f ) / 100.0f;
			float hzInt = idMath::Floor( engineHz );
			float hzFrac = idMath::Frac( engineHz ) * 100.0f;
			hzFrac = idMath::Rint( hzFrac + adjustAmount );
			hzFrac = idMath::ClampFloat( 0.0f, 99.0f, hzFrac );
			hzFrac /= 100.0f;
			engineHz = hzInt + hzFrac;
			if( idMath::Frac( engineHz ) == 0.0f )
			{
				cvarSystem->SetCVarFloat( "com_engineHz", engineHz );
			}
			else
			{
				cvarSystem->SetCVarString( "com_engineHz", va( "%.2f", engineHz ) );
			}
			break;
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_HIGH_RESOLUTION_CLOCK:
		{
			static const int numValues = 3;
			static const int values[numValues] = { 0, 1, 2 };
			cvarSystem->SetCVarInteger( "com_hiResClock", AdjustOption( cvarSystem->GetCVarInteger( "com_hiResClock" ), values, numValues, adjustAmount ) );
			break;
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_VSYNC:
		{
			static const int numValues = 3;
			static const int values[numValues] = { 0, 1, 2 };
			cvarSystem->SetCVarInteger( "r_swapInterval", AdjustOption( cvarSystem->GetCVarInteger( "r_swapInterval" ), values, numValues, adjustAmount ) );
			break;
		}
	}
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::GetField
========================
*/
idSWFScriptVar idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::GetField( const int fieldIndex ) const
{
	if( fieldIndex < 0 || fieldIndex >= MAX_SYSTEM_FIELDS )
	{
		return false;
	}
	switch( fieldIndex )
	{
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_FULLSCREEN:
		{
			const int fs = cvarSystem->GetCVarInteger( "r_vidFullscreen" );
			if( fs == -1 )
			{
				return "Borderless";
			}
			if( fs == 0 )
			{
				return "Windowed";
			}
			return "Fullscreen";
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_RESOLUTION:
		{
			return va( "%dx%d", cvarSystem->GetCVarInteger( "r_windowWidth" ), cvarSystem->GetCVarInteger( "r_windowHeight" ) );
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_REFRESH_RATE:
		{
			const int rate = cvarSystem->GetCVarInteger( "r_vidDisplayRefresh" );
			if( rate == 0 )
			{
				return "Auto";
			}
			return va( "%d Hz", rate );
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_INT:
		{
			float engineHz = cvarSystem->GetCVarFloat( "com_engineHz" );
			engineHz = idMath::Rint( engineHz * 100.0f ) / 100.0f;
			return va( "%.2f FPS", engineHz );
		}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_FRAC:
			return "              "; // leave blank here and let idMenuDataSource_SystemSettings::SYSTEM_FIELD_FRAMERATE_INT show the complete framerate value
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_HIGH_RESOLUTION_CLOCK:
			if( cvarSystem->GetCVarInteger( "com_hiResClock" ) == 1 )
			{
				return "Normal";
			}
			else if( cvarSystem->GetCVarInteger( "com_hiResClock" ) == 2 )
			{
				return "Affinity";
			}
			else
			{
				return "#str_swf_disabled";
			}
		case idMenuDataSource_SystemSettings::SYSTEM_FIELD_VSYNC:
			if( cvarSystem->GetCVarInteger( "r_swapInterval" ) == 1 )
			{
				return "#str_swf_smart";
			}
			else if( cvarSystem->GetCVarInteger( "r_swapInterval" ) == 2 )
			{
				return "#str_swf_enabled";
			}
			else
			{
				return "#str_swf_disabled";
			}
	}
	return false;
}

/*
========================
idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_SystemOptions::idMenuDataSource_SystemSettings::IsDataChanged() const
{
	if( originalFramerate != cvarSystem->GetCVarFloat( "com_engineHz" ) ||
			originalHighResolutionClock != cvarSystem->GetCVarInteger( "com_hiResClock" ) ||
			originalVsync != cvarSystem->GetCVarInteger( "r_swapInterval" ) )
	{
		return true;
	}
	return false;
}

/*
========================
idMenuWidget_SystemOptionsList::Update
========================
*/
void idMenuWidget_SystemOptionsList::Update()
{
	if( GetSWFObject() == NULL )
	{
		return;
	}

	idSWFScriptObject& root = GetSWFObject()->GetRootObject();

	if( !BindSprite( root ) )
	{
		return;
	}

	//idLib::Printf( "SystemOptionsList::Update( offset = %i )\n", GetViewOffset() );

	// clear old sprites and rebuild the options
	for( int childIndex = 0; childIndex < GetTotalNumberOfOptions(); ++childIndex )
	{
		idMenuWidget& child = GetChildByIndex( childIndex );

		child.ClearSprite();
	}

	for( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex )
	{
		if( optionIndex >= children.Num() )
		{
			// not enough children
			idSWFSpriteInstance* item = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", optionIndex ) );
			if( item != NULL )
			{
				item->SetVisible( false );
				continue;
			}
		}

		// account view offset and total number of options
		const int childIndex = ( GetViewOffset() + optionIndex ) % GetTotalNumberOfOptions();
		idMenuWidget& child = GetChildByIndex( childIndex );

		child.SetSpritePath( GetSpritePath(), va( "item%d", optionIndex ) );
		if( child.BindSprite( root ) )
		{
			if( optionIndex >= GetTotalNumberOfOptions() )
			{
				child.ClearSprite();
				continue;
			}

			child.Update();

			if( optionIndex == focusIndex )
			{
				child.SetState( WIDGET_STATE_SELECTING );
			}
			else
			{
				child.SetState( WIDGET_STATE_NORMAL );
			}
		}
	}

	idSWFSpriteInstance* const upSprite = GetSprite()->GetScriptObject()->GetSprite( "upIndicator" );
	if( upSprite != NULL )
	{
		upSprite->SetVisible( GetViewOffset() > 0 );
	}

	idSWFSpriteInstance* const downSprite = GetSprite()->GetScriptObject()->GetSprite( "downIndicator" );
	if( downSprite != NULL )
	{
		downSprite->SetVisible( GetViewOffset() + GetNumVisibleOptions() < GetTotalNumberOfOptions() );
	}
}

/*
========================
idMenuWidget_SystemOptionsList::Scroll
========================
*/
void idMenuWidget_SystemOptionsList::Scroll( const int scrollAmount, const bool wrapAround )
{
	if( GetTotalNumberOfOptions() == 0 )
	{
		return;
	}

	int newIndex, newOffset;

	// RB: always wrap around
	CalculatePositionFromIndexDelta( newIndex, newOffset, GetViewIndex(), GetViewOffset(), GetNumVisibleOptions(), GetTotalNumberOfOptions(), scrollAmount, IsWrappingAllowed(), true ); //wrapAround );

	//int oldViewIndex = GetViewIndex();
	//int oldViewOffset = GetViewOffset();
	int oldFocusIndex = GetFocusIndex();

	if( newOffset != GetViewOffset() )
	{
		SetViewOffset( newOffset );
		if( menuData != NULL )
		{
			menuData->PlaySound( GUI_SOUND_FOCUS );
		}

		// RB: HACK and I don't like it.
		// focusIndex is used here for the visible state and not for event handling.
		focusIndex = newIndex;
		Update();
		focusIndex = oldFocusIndex;
	}

	if( newIndex != GetViewIndex() )
	{
		SetViewIndex( newIndex );

		// trigger focus/unfocus sprite actions
		SetFocusIndex( newIndex );// - newOffset );
	}

	//idLib::Printf( "scroll = %i, index = %i -> %i, offset = %i -> %i, focus = %i -> %i\n", scrollAmount, oldViewIndex, newIndex, oldViewOffset, newOffset, oldFocusIndex, GetFocusIndex() );
}
