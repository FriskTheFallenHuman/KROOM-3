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

const static int NUM_GRAPHICS_OPTIONS = 8;

/*
========================
idMenuScreen_Shell_GraphicsOptions::Initialize
========================
*/
void idMenuScreen_Shell_GraphicsOptions::Initialize( idMenuHandler* data )
{
	idMenuScreen::Initialize( data );

	if( data != NULL )
	{
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuGameOptions" );

	options = new( TAG_SWF ) idMenuWidget_SystemOptionsList(); // RB: allow more options than defined in the SWF
	options->SetNumVisibleOptions( NUM_GRAPHICS_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	options->SetControlList( true );
	options->Initialize( data );
	AddChild( options );

	btnBack = new( TAG_SWF ) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_settings" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );
	AddChild( btnBack );

	idMenuWidget_ControlButton* control;
	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_04128" );
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_ANTIALIASING );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_ANTIALIASING );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Filmic VFX" );
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_POSTFX );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_POSTFX );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "SSAO" );
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_SSAO );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_SSAO );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "#str_swf_motionblur" );
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_MOTIONBLUR );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_MOTIONBLUR );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_swf_lodbias" );
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_LODBIAS );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_LODBIAS );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "Ambient Lighting" );
	control->SetDescription( "Sets the amount of indirect lighting. Needed for modern PBR reflections" );
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_AMBIENT_BRIGHTNESS );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_SSAO );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "#str_02155" );	// Brightness
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_BRIGHTNESS );
	control->SetupEvents( 2, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_BRIGHTNESS );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_BAR );
	control->SetLabel( "Soft-Shadows LOD" ); //Soft ShadowsLOD
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_SHADOWMAP_LOD );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_SHADOWMAP_LOD );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "HDR" ); //HDR
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_HDR );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_HDR );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "HDR Auto-Exposure" ); //HDR Auto-Exposure
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_HDR_AUTOEXPOSURE );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_HDR_AUTOEXPOSURE );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "SSGI" );	// SSGI
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_SSGI );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_SSGI );
	options->AddChild( control );

	control = new( TAG_SWF ) idMenuWidget_ControlButton();
	control->SetOptionType( OPTION_SLIDER_TEXT );
	control->SetLabel( "Half-Lambert Lighting" ); //Half-Lambert Lighting
	control->SetDataSource( &renderData, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_HALF_LIGHT );
	control->SetupEvents( DEFAULT_REPEAT_TIME, options->GetChildren().Num() );
	control->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, idMenuDataSource_GraphicsSettings::GRAPHICS_FIELD_HALF_LIGHT );
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
idMenuScreen_Shell_GraphicsOptions::Update
========================
*/
void idMenuScreen_Shell_GraphicsOptions::Update()
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
			heading->SetText( "GRAPHICS SETTINGS" );	// GRAPHICS SETTINGS
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
idMenuScreen_Shell_GraphicsOptions::ShowScreen
========================
*/
void idMenuScreen_Shell_GraphicsOptions::ShowScreen( const mainMenuTransition_t transitionType )
{
	renderData.LoadData();
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_GraphicsOptions::HideScreen
========================
*/
void idMenuScreen_Shell_GraphicsOptions::HideScreen( const mainMenuTransition_t transitionType )
{
	if( renderData.IsDataChanged() )
	{
		renderData.CommitData();
	}
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_GraphicsOptions::HandleAction h
========================
*/
bool idMenuScreen_Shell_GraphicsOptions::HandleAction( idWidgetAction& action, const idWidgetEvent& event, idMenuWidget* widget, bool forceHandled )
{

	if( menuData == NULL )
	{
		return true;
	}

	if( menuData->ActiveScreen() != SHELL_AREA_GRAPHICS_OPTIONS )
	{
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList& parms = action.GetParms();

	switch( actionType )
	{
		case WIDGET_ACTION_GO_BACK:
		{
			menuData->SetNextScreen( SHELL_AREA_SETTINGS, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED:
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

			if( selectionIndex != options->GetFocusIndex() )
			{
				options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				options->SetFocusIndex( selectionIndex );
			}

			renderData.AdjustField( selectionIndex, 1 );
			options->Update();

			return true;
		}
		case WIDGET_ACTION_START_REPEATER:
		{
			if( parms.Num() == 4 )
			{
				int selectionIndex = parms[3].ToInteger();
				if( selectionIndex != options->GetFocusIndex() )
				{
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
					options->SetFocusIndex( selectionIndex );
				}
			}
			break;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/////////////////////////////////
// SCREEN SETTINGS
/////////////////////////////////

extern idCVar r_shadowMapLodScale;
extern idCVar r_useHDR;
extern idCVar r_hdrAutoExposure;
extern idCVar r_antiAliasing;
extern idCVar r_motionBlur;
extern idCVar r_useFilmicPostProcessing;
extern idCVar r_exposure; // RB: use this to control HDR exposure or brightness in LDR mode
extern idCVar r_lodBias;
extern idCVar r_useSSGI;
extern idCVar r_useHalfLambertLighting;
extern idCVar r_shadowMapLodScale;

/*
========================
idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_AudioSettings::idMenuDataSource_AudioSettings
========================
*/
idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_GraphicsSettings::idMenuDataSource_GraphicsSettings()
{
	fields.SetNum( MAX_GRAPHICS_FIELDS );
	originalFields.SetNum( MAX_GRAPHICS_FIELDS );
}

/*
========================
idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_AudioSettings::LoadData
========================
*/
void idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_GraphicsSettings::LoadData()
{
	fields[ GRAPHICS_FIELD_ANTIALIASING ].SetInteger( r_antiAliasing.GetInteger() );
	fields[ GRAPHICS_FIELD_BRIGHTNESS ].SetFloat( r_exposure.GetFloat() );
	fields[ GRAPHICS_FIELD_SSAO ].SetBool( r_useSSAO.GetBool() );
	fields[ GRAPHICS_FIELD_POSTFX ].SetBool( r_useFilmicPostProcessing.GetBool() );
	fields[ GRAPHICS_FIELD_AMBIENT_BRIGHTNESS ].SetFloat( r_forceAmbient.GetFloat() );
	fields[ GRAPHICS_FIELD_LODBIAS ].SetFloat( r_lodBias.GetFloat() );
	fields[ GRAPHICS_FIELD_MOTIONBLUR ].SetInteger( r_motionBlur.GetInteger() );
	fields[ GRAPHICS_FIELD_HDR ].SetBool( r_useHDR.GetBool() );
	fields[ GRAPHICS_FIELD_HDR_AUTOEXPOSURE ].SetBool( r_hdrAutoExposure.GetBool() );
	fields[ GRAPHICS_FIELD_SHADOWMAP_LOD ].SetFloat( r_shadowMapLodScale.GetFloat() );
	fields[ GRAPHICS_FIELD_SSGI ].SetBool( r_useSSGI.GetBool() );
	fields[ GRAPHICS_FIELD_HALF_LIGHT ].SetBool( r_useHalfLambertLighting.GetBool() );

	originalFields = fields;
}

/*
========================
idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_AudioSettings::CommitData
========================
*/
void idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_GraphicsSettings::CommitData()
{
	r_antiAliasing.SetInteger( fields[ GRAPHICS_FIELD_ANTIALIASING ].ToInteger() );
	r_exposure.SetFloat( fields[ GRAPHICS_FIELD_BRIGHTNESS ].ToFloat() );
	r_useSSAO.SetBool( fields[ GRAPHICS_FIELD_SSAO ].ToBool() );
	r_useFilmicPostProcessing.SetBool( fields[ GRAPHICS_FIELD_POSTFX ].ToBool() );
	r_forceAmbient.SetFloat( fields[ GRAPHICS_FIELD_AMBIENT_BRIGHTNESS ].ToFloat() );
	r_lodBias.SetFloat( fields[ GRAPHICS_FIELD_LODBIAS ].ToFloat() );
	r_motionBlur.SetInteger( fields[ GRAPHICS_FIELD_MOTIONBLUR ].ToInteger() );
	r_useHDR.SetBool( fields[ GRAPHICS_FIELD_HDR ].ToBool() );
	r_hdrAutoExposure.SetBool( fields[ GRAPHICS_FIELD_HDR_AUTOEXPOSURE ].ToBool() );
	r_shadowMapLodScale.SetFloat( fields[ GRAPHICS_FIELD_SHADOWMAP_LOD ].ToFloat() );
	r_useSSGI.SetBool( fields[ GRAPHICS_FIELD_SSGI ].ToBool() );
	r_useHalfLambertLighting.SetBool( fields[ GRAPHICS_FIELD_HALF_LIGHT ].ToBool() );

	cvarSystem->SetModifiedFlags( CVAR_ARCHIVE );

	// make the committed fields into the backup fields
	originalFields = fields;
}

extern int AdjustOption( int currentValue, const int values[], int numValues, int adjustment );
extern float LinearAdjust( float input, float currentMin, float currentMax, float desiredMin, float desiredMax );

/*
========================
idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_AudioSettings::AdjustField
========================
*/
void idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_GraphicsSettings::AdjustField( const int fieldIndex, const int adjustAmount )
{
	if( fieldIndex == GRAPHICS_FIELD_SHADOWMAP_LOD )
	{
		fields[ fieldIndex ].SetFloat( fields[ GRAPHICS_FIELD_SHADOWMAP_LOD ].ToFloat() + adjustAmount * 0.1f );
	}
	else if( fieldIndex == GRAPHICS_FIELD_ANTIALIASING )
	{
		// RB: disabled 16x MSAA
		static const int numValues = 5;
		static const int values[numValues] =
		{
			ANTI_ALIASING_NONE,
			ANTI_ALIASING_SMAA_1X,
			ANTI_ALIASING_MSAA_2X,
			ANTI_ALIASING_MSAA_4X,
			ANTI_ALIASING_MSAA_8X
		};
		// RB end

		fields[ fieldIndex ].SetInteger( AdjustOption( fields[ GRAPHICS_FIELD_ANTIALIASING ].ToInteger(), values, numValues, adjustAmount ) );
	}
	else if( fieldIndex == GRAPHICS_FIELD_LODBIAS )
	{
		const float percent = LinearAdjust( fields[ GRAPHICS_FIELD_LODBIAS ].ToFloat(), -1.0f, 1.0f, 0.0f, 100.0f );
		const float adjusted = percent + ( float )adjustAmount * 5.0f;
		const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );

		fields[ fieldIndex ].SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, -1.0f, 1.0f ) );
	}
	else if( fieldIndex == GRAPHICS_FIELD_AMBIENT_BRIGHTNESS )
	{
		const float percent = LinearAdjust( fields[ GRAPHICS_FIELD_AMBIENT_BRIGHTNESS ].ToFloat(), 0.0f, 1.0f, 0.0f, 100.0f );
		const float adjusted = percent + ( float )adjustAmount;
		const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );


		fields[ fieldIndex ].SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, 0.0f, 1.0f ) );

	}
	else if( fieldIndex == GRAPHICS_FIELD_BRIGHTNESS )
	{
		const float percent = LinearAdjust( fields[ GRAPHICS_FIELD_BRIGHTNESS ].ToFloat(), 0.0f, 1.0f, 0.0f, 100.0f );
		const float adjusted = percent + ( float )adjustAmount;
		const float clamped = idMath::ClampFloat( 0.0f, 100.0f, adjusted );

		fields[ fieldIndex ].SetFloat( LinearAdjust( clamped, 0.0f, 100.0f, 0.0f, 1.0f ) );
	}
	else if( fieldIndex == GRAPHICS_FIELD_MOTIONBLUR )
	{
		static const int numValues = 5;
		static const int values[numValues] = { 0, 2, 3, 4, 5 };
		fields[ fieldIndex ].SetInteger( AdjustOption( fields[ GRAPHICS_FIELD_MOTIONBLUR ].ToInteger(), values, numValues, adjustAmount ) );
	}
	else
	{
		fields[ fieldIndex ].SetBool( !fields[ fieldIndex ].ToBool() );
	}
}

/*
========================
idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_AudioSettings::GetField
========================
*/
idSWFScriptVar idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_GraphicsSettings::GetField( const int fieldIndex ) const
{
	if( fieldIndex == GRAPHICS_FIELD_SHADOWMAP_LOD )
	{
		return LinearAdjust( fields[ GRAPHICS_FIELD_SHADOWMAP_LOD ].ToFloat(), 0.1f, 2.0f, 0.0f, 100.0f );
	}
	else if( fieldIndex == GRAPHICS_FIELD_HDR )
	{
		if( fields[ GRAPHICS_FIELD_HDR ].ToBool() == true )
		{
			return "#str_swf_enabled";
		}
		else
		{
			return "#str_swf_disabled";
		}
	}
	else if( fieldIndex == GRAPHICS_FIELD_HDR_AUTOEXPOSURE )
	{
		if( fields[ GRAPHICS_FIELD_HDR_AUTOEXPOSURE ].ToBool() == true )
		{
			return "#str_swf_enabled";
		}
		else
		{
			return "#str_swf_disabled";
		}
	}
	else if( fieldIndex == GRAPHICS_FIELD_SSGI )
	{
		if( fields[ GRAPHICS_FIELD_SSGI ].ToBool() == true )
		{
			return "#str_swf_enabled";
		}
		else
		{
			return "#str_swf_disabled";
		}
	}
	else if( fieldIndex == GRAPHICS_FIELD_HALF_LIGHT )
	{
		if( fields[ GRAPHICS_FIELD_HALF_LIGHT ].ToBool() == true )
		{
			return "#str_swf_enabled";
		}
		else
		{
			return "#str_swf_disabled";
		}
	}
	else if( fieldIndex == GRAPHICS_FIELD_ANTIALIASING )
	{
		if( fields[ GRAPHICS_FIELD_ANTIALIASING ].ToInteger() == 0 )
		{
			return "#str_swf_disabled";
		}

		static const int numValues = 5;
		static const char* values[numValues] =
		{
			"None",
			"SMAA 1X",
			"MSAA 2X",
			"MSAA 4X",
			"MSAA 8X"
		};

		compile_time_assert( numValues == ( ANTI_ALIASING_MSAA_8X + 1 ) );

		return values[ fields[ GRAPHICS_FIELD_ANTIALIASING ].ToInteger() ];
	}
	else if( fieldIndex == GRAPHICS_FIELD_POSTFX )
	{
		if( fields[ GRAPHICS_FIELD_POSTFX ].ToBool() == true )
		{
			return "#str_swf_enabled";
		}
		else
		{
			return "#str_swf_disabled";
		}
	}
	else if( fieldIndex == GRAPHICS_FIELD_LODBIAS )
	{
		return LinearAdjust( fields[ GRAPHICS_FIELD_LODBIAS ].ToFloat(), -1.0f, 1.0f, 0.0f, 100.0f );
	}
	else if( fieldIndex == GRAPHICS_FIELD_SSAO )
	{
		if( fields[ GRAPHICS_FIELD_SSAO ].ToBool() == true )
		{
			return "#str_swf_enabled";
		}
		else
		{
			return "#str_swf_disabled";
		}
	}
	else if( fieldIndex == GRAPHICS_FIELD_AMBIENT_BRIGHTNESS )
	{
		return LinearAdjust( fields[ GRAPHICS_FIELD_AMBIENT_BRIGHTNESS ].ToFloat(), 0.0f, 1.0f, 0.0f, 100.0f );
	}
	else if( fieldIndex == GRAPHICS_FIELD_BRIGHTNESS )
	{
		return LinearAdjust( fields[ GRAPHICS_FIELD_BRIGHTNESS ].ToFloat(), 0.0f, 1.0f, 0.0f, 100.0f );
	}
	else if( fieldIndex == GRAPHICS_FIELD_MOTIONBLUR )
	{
		if( fields[ GRAPHICS_FIELD_MOTIONBLUR ].ToInteger() == 0 )
		{
			return "#str_swf_disabled";
		}
		return va( "%dx", idMath::IPow( 2, fields[ GRAPHICS_FIELD_MOTIONBLUR ].ToInteger() ) );
	}

	return fields[ fieldIndex ];
}

/*
========================
idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_AudioSettings::IsDataChanged
========================
*/
bool idMenuScreen_Shell_GraphicsOptions::idMenuDataSource_GraphicsSettings::IsDataChanged() const
{

	if( fields[ GRAPHICS_FIELD_ANTIALIASING ].ToInteger() != originalFields[ GRAPHICS_FIELD_ANTIALIASING ].ToInteger() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_BRIGHTNESS ].ToFloat() != originalFields[ GRAPHICS_FIELD_BRIGHTNESS ].ToFloat() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_SSAO ].ToBool() != originalFields[ GRAPHICS_FIELD_SSAO ].ToBool() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_POSTFX ].ToBool() != originalFields[ GRAPHICS_FIELD_POSTFX ].ToBool() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_AMBIENT_BRIGHTNESS ].ToFloat() != originalFields[ GRAPHICS_FIELD_AMBIENT_BRIGHTNESS ].ToFloat() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_LODBIAS ].ToFloat() != originalFields[ GRAPHICS_FIELD_LODBIAS ].ToFloat() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_MOTIONBLUR ].ToInteger() != originalFields[ GRAPHICS_FIELD_MOTIONBLUR ].ToInteger() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_SHADOWMAP_LOD ].ToFloat() != originalFields[ GRAPHICS_FIELD_SHADOWMAP_LOD ].ToFloat() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_HDR ].ToBool() != originalFields[ GRAPHICS_FIELD_HDR ].ToBool() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_HDR_AUTOEXPOSURE ].ToBool() != originalFields[ GRAPHICS_FIELD_HDR_AUTOEXPOSURE ].ToBool() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_SSGI ].ToBool() != originalFields[ GRAPHICS_FIELD_SSGI ].ToBool() )
	{
		return true;
	}

	if( fields[ GRAPHICS_FIELD_HALF_LIGHT ].ToBool() != originalFields[ GRAPHICS_FIELD_HALF_LIGHT ].ToBool() )
	{
		return true;
	}

	return false;
}
