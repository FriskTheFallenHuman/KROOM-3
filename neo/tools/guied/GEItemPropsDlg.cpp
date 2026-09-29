/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 1999-2011 Raven Software

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

#include "GEApp.h"

/*
================
WX

Converts an idStr to a wxString via UTF-8.
================
*/
static inline wxString WX( const idStr& s )
{
	return wxString::FromUTF8( s.c_str() );
}

/*
================
BuildExcludeChars

Characters disallowed by text fields (name, key).
================
*/
static wxArrayString BuildExcludeChars()
{
	wxArrayString ex;
	ex.Add( " " );
	ex.Add( "\t" );
	ex.Add( "\r" );
	ex.Add( "\n" );
	return ex;
}

/*
================
rvGEAlphaButton::rvGEAlphaButton
================
*/
rvGEAlphaButton::rvGEAlphaButton( wxWindow* parent, wxWindowID id, float alpha )
	: wxButton( parent, id, "", wxDefaultPosition, wxSize( 32, 22 ) )
	, m_alpha( alpha )
{
	SetBackgroundColour( wxColour( ( unsigned char )( alpha * 255 ),
								   ( unsigned char )( alpha * 255 ),
								   ( unsigned char )( alpha * 255 ) ) );
	Bind( wxEVT_BUTTON, &rvGEAlphaButton::OnClick, this );
}

/*
================
rvGEAlphaButton::SetAlpha
================
*/
void rvGEAlphaButton::SetAlpha( float a )
{
	m_alpha = wxMin( 1.0f, wxMax( 0.0f, a ) );
	Refresh();
}

/*
================
rvGEAlphaButton::GetAlpha
================
*/
float rvGEAlphaButton::GetAlpha() const
{
	return m_alpha;
}

/*
================
rvGEAlphaButton::OnClick

Pops up a slider dialog to pick a new alpha value.
================
*/
void rvGEAlphaButton::OnClick( wxCommandEvent& )
{
	wxDialog dlg( this, wxID_ANY, "Alpha", wxDefaultPosition, wxSize( 260, 80 ), wxDEFAULT_DIALOG_STYLE );
	wxSlider* slider = new wxSlider( &dlg, wxID_ANY, ( int )( m_alpha * 255 ), 0, 255, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( slider, 1, wxEXPAND | wxALL, 8 );
	sizer->Add( dlg.CreateButtonSizer( wxOK | wxCANCEL ), 0, wxEXPAND | wxALL, 4 );
	dlg.SetSizer( sizer );

	if( dlg.ShowModal() == wxID_OK )
	{
		SetAlpha( slider->GetValue() / 255.0f );
	}
}

/*
================
rvGEItemPropsGeneralPage::rvGEItemPropsGeneralPage

Builds the "General" page: name, type, visibility flags.
================
*/
rvGEItemPropsGeneralPage::rvGEItemPropsGeneralPage( wxWindow* parent, idDict* dict, rvGEWindowWrapper::EWindowType type )
	: wxPanel( parent )
	, m_dict( dict )
	, m_type( type )
{
	wxFlexGridSizer* sizer = new wxFlexGridSizer( 2, 6, 8 );
	sizer->AddGrowableCol( 1, 1 );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Name:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_name = new wxTextCtrl( this, wxID_ANY );
	m_name->SetValidator( wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST ) );
	static_cast<wxTextValidator*>( m_name->GetValidator() )->SetExcludes( BuildExcludeChars() );
	sizer->Add( m_name, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Type:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_typeLabel = new wxStaticText( this, wxID_ANY, rvGEWindowWrapper::WindowTypeToString( type ) );
	sizer->Add( m_typeLabel, 1, wxEXPAND );

	sizer->AddSpacer( 4 );
	sizer->AddSpacer( 4 );

	m_visible = new wxCheckBox( this, wxID_ANY, "Visible" );
	m_noEvents = new wxCheckBox( this, wxID_ANY, "No Events" );
	m_noTime = new wxCheckBox( this, wxID_ANY, "No Time" );
	m_noClip = new wxCheckBox( this, wxID_ANY, "No Clip" );
	m_noCursor = new wxCheckBox( this, wxID_ANY, "No Cursor" );

	sizer->Add( m_visible, 0 );
	sizer->Add( m_noEvents, 0 );
	sizer->Add( m_noTime, 0 );
	sizer->Add( m_noClip, 0 );
	sizer->Add( m_noCursor, 0 );

	SetSizer( sizer );
}

/*
================
rvGEItemPropsGeneralPage::SetActive

Reads the current dictionary into the controls.
================
*/
bool rvGEItemPropsGeneralPage::SetActive()
{
	if( !m_dict )
	{
		return false;
	}

	m_name->ChangeValue( WX( idStr( m_dict->GetString( "name", "unnamed" ) ).StripQuotes() ) );

	const bool visEnable = !IsExpression( m_dict->GetString( "visible", "1" ) );
	m_visible->SetValue( atol( idStr( m_dict->GetString( "visible", "1" ) ).StripQuotes() ) != 0 );
	m_visible->Enable( visEnable );

	struct
	{
		const char* key;
		wxCheckBox* box;
	} checkboxes[] =
	{
		{ "notime", m_noTime },
		{ "noevents", m_noEvents },
		{ "noclip", m_noClip },
		{ "nocursor", m_noCursor },
	};
	for( auto& cb : checkboxes )
	{
		const bool enable = !IsExpression( m_dict->GetString( cb.key, "0" ) );
		cb.box->SetValue( atol( idStr( m_dict->GetString( cb.key, "0" ) ).StripQuotes() ) != 0 );
		cb.box->Enable( enable );
	}

	return true;
}

/*
================
rvGEItemPropsGeneralPage::KillActive

Writes the controls back into the dictionary.
================
*/
bool rvGEItemPropsGeneralPage::KillActive()
{
	if( !m_dict )
	{
		return false;
	}

	m_dict->Set( "name", m_name->GetValue().utf8_str() );

	auto setBool = [this]( const char* key, const wxCheckBox * box, bool writeDefault )
	{
		if( !box->IsEnabled() )
		{
			return;
		}

		if( box->GetValue() )
		{
			m_dict->Set( key, "1" );
		}
		else if( writeDefault )
		{
			m_dict->Set( key, "0" );
		}
		else
		{
			m_dict->Delete( key );
		}
	};

	setBool( "visible", m_visible, true );
	setBool( "notime", m_noTime, false );
	setBool( "noevents", m_noEvents, false );
	setBool( "noclip", m_noClip, false );
	setBool( "nocursor", m_noCursor, false );

	return true;
}

/*
================
rvGEItemPropsImagePage::rvGEItemPropsImagePage

Builds the "Image" page: backcolor, material, border, matcolor.
================
*/
rvGEItemPropsImagePage::rvGEItemPropsImagePage( wxWindow* parent, idDict* dict )
	: wxPanel( parent )
	, m_dict( dict )
{
	wxFlexGridSizer* sizer = new wxFlexGridSizer( 2, 6, 6 );
	sizer->AddGrowableCol( 1, 1 );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Backcolor:" ), 0, wxALIGN_CENTER_VERTICAL );
	wxBoxSizer* backRow = new wxBoxSizer( wxHORIZONTAL );
	m_useBackcolor = new wxCheckBox( this, wxID_ANY, "" );
	m_backColor = new wxColourPickerCtrl( this, wxID_ANY );
	m_backAlpha = new rvGEAlphaButton( this, wxID_ANY );
	backRow->Add( m_useBackcolor, 0, wxALIGN_CENTER_VERTICAL );
	backRow->Add( m_backColor, 1, wxEXPAND );
	backRow->Add( m_backAlpha, 0 );
	sizer->Add( backRow, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Material:" ), 0, wxALIGN_CENTER_VERTICAL );
	wxBoxSizer* matRow = new wxBoxSizer( wxHORIZONTAL );
	m_useMaterial = new wxCheckBox( this, wxID_ANY, "" );
	m_background = new wxTextCtrl( this, wxID_ANY );
	matRow->Add( m_useMaterial, 0, wxALIGN_CENTER_VERTICAL );
	matRow->Add( m_background, 1, wxEXPAND );
	sizer->Add( matRow, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "X Scale:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_matScaleX = new wxTextCtrl( this, wxID_ANY );
	m_matScaleX->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );
	sizer->Add( m_matScaleX, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Y Scale:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_matScaleY = new wxTextCtrl( this, wxID_ANY );
	m_matScaleY->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );
	sizer->Add( m_matScaleY, 1, wxEXPAND );

	sizer->AddSpacer( 4 );
	m_variableBackground = new wxCheckBox( this, wxID_ANY, "Disable Cache (variablebackground)" );
	sizer->Add( m_variableBackground, 0 );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Border Size:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_borderSize = new wxSpinCtrl( this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 512, 0 );
	sizer->Add( m_borderSize, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Border:" ), 0, wxALIGN_CENTER_VERTICAL );
	wxBoxSizer* borderRow = new wxBoxSizer( wxHORIZONTAL );
	m_useBorderColor = new wxRadioButton( this, wxID_ANY, "Color", wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_useBorderMaterial = new wxRadioButton( this, wxID_ANY, "Material" );
	borderRow->Add( m_useBorderColor, 0, wxALIGN_CENTER_VERTICAL );
	borderRow->Add( m_useBorderMaterial, 1, wxALIGN_CENTER_VERTICAL );
	sizer->Add( borderRow, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Bordercolor:" ), 0, wxALIGN_CENTER_VERTICAL );
	wxBoxSizer* bcRow = new wxBoxSizer( wxHORIZONTAL );
	m_borderColor = new wxColourPickerCtrl( this, wxID_ANY );
	m_borderAlpha = new rvGEAlphaButton( this, wxID_ANY );
	bcRow->Add( m_borderColor, 1, wxEXPAND );
	bcRow->Add( m_borderAlpha, 0 );
	sizer->Add( bcRow, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Bordermaterial:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_borderMaterial = new wxTextCtrl( this, wxID_ANY );
	sizer->Add( m_borderMaterial, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Matcolor:" ), 0, wxALIGN_CENTER_VERTICAL );
	wxBoxSizer* mcRow = new wxBoxSizer( wxHORIZONTAL );
	m_matColor = new wxColourPickerCtrl( this, wxID_ANY );
	m_matAlpha = new rvGEAlphaButton( this, wxID_ANY );
	mcRow->Add( m_matColor, 1, wxEXPAND );
	mcRow->Add( m_matAlpha, 0 );
	sizer->Add( mcRow, 1, wxEXPAND );

	SetSizer( sizer );

	m_useBackcolor->Bind( wxEVT_CHECKBOX, &rvGEItemPropsImagePage::OnToggle, this );
	m_useMaterial->Bind( wxEVT_CHECKBOX, &rvGEItemPropsImagePage::OnToggle, this );
	m_useBorderColor->Bind( wxEVT_RADIOBUTTON, &rvGEItemPropsImagePage::OnToggle, this );
	m_useBorderMaterial->Bind( wxEVT_RADIOBUTTON, &rvGEItemPropsImagePage::OnToggle, this );
}

/*
================
rvGEItemPropsImagePage::OnToggle
================
*/
void rvGEItemPropsImagePage::OnToggle( wxCommandEvent& )
{
	UpdateCheckedStates();
}

/*
================
rvGEItemPropsImagePage::UpdateCheckedStates

Enables/disables child controls based on current checkbox states.
================
*/
void rvGEItemPropsImagePage::UpdateCheckedStates()
{
	const bool backColorEnabled = !IsExpression( m_dict->GetString( "backcolor", "1,1,1,1" ) );
	m_useBackcolor->Enable( backColorEnabled );
	m_backColor->Enable( backColorEnabled && m_useBackcolor->GetValue() );
	m_backAlpha->Enable( backColorEnabled && m_useBackcolor->GetValue() );

	const bool useMat = m_useMaterial->GetValue();
	m_background->Enable( useMat );
	m_variableBackground->Enable( useMat );
	m_matScaleX->Enable( useMat );
	m_matScaleY->Enable( useMat );

	const bool hasBorder = m_borderSize->GetValue() > 0;
	const bool borderColorEnabled = !IsExpression( m_dict->GetString( "bordercolor", "1,1,1,1" ) );
	m_useBorderColor->Enable( hasBorder && borderColorEnabled );
	m_useBorderMaterial->Enable( hasBorder && borderColorEnabled );

	const bool useBorderColor = m_useBorderColor->GetValue();
	m_borderColor->Enable( hasBorder && borderColorEnabled && useBorderColor );
	m_borderAlpha->Enable( hasBorder && borderColorEnabled && useBorderColor );
	m_borderMaterial->Enable( hasBorder && borderColorEnabled && !useBorderColor );

	const bool matColorEnabled = !IsExpression( m_dict->GetString( "matcolor", "1,1,1,1" ) );
	const bool matNeeded = ( hasBorder && !useBorderColor ) || useMat;
	m_matColor->Enable( matColorEnabled && matNeeded );
	m_matAlpha->Enable( matColorEnabled && matNeeded );
}

/*
================
rvGEItemPropsImagePage::SetActive
================
*/
bool rvGEItemPropsImagePage::SetActive()
{
	auto setColor = []( wxColourPickerCtrl * ctrl, rvGEAlphaButton * alphaBtn, const idVec4 & v )
	{
		ctrl->SetColour( wxColour(
							 ( unsigned char )( idMath::ClampFloat( 0.0f, 1.0f, v.x ) * 255 ),
							 ( unsigned char )( idMath::ClampFloat( 0.0f, 1.0f, v.y ) * 255 ),
							 ( unsigned char )( idMath::ClampFloat( 0.0f, 1.0f, v.z ) * 255 ) ) );
		if( alphaBtn )
		{
			alphaBtn->SetAlpha( v.w );
		}
	};

	auto parseVec4 = []( const char* s ) -> idVec4
	{
		idVec4 v( 1, 1, 1, 1 );
		sscanf( s, "%f,%f,%f,%f", &v.x, &v.y, &v.z, &v.w );
		return v;
	};

	setColor( m_backColor, m_backAlpha, parseVec4( m_dict->GetString( "backcolor", "1,1,1,1" ) ) );
	setColor( m_matColor, m_matAlpha, parseVec4( m_dict->GetString( "matcolor", "1,1,1,1" ) ) );
	setColor( m_borderColor, m_borderAlpha, parseVec4( m_dict->GetString( "bordercolor", "0,0,0,1" ) ) );

	m_background->ChangeValue( WX( idStr( m_dict->GetString( "background", "" ) ).StripQuotes() ) );
	m_borderMaterial->ChangeValue( WX( idStr( m_dict->GetString( "borderShader", "" ) ).StripQuotes() ) );
	m_borderSize->SetValue( atoi( idStr( m_dict->GetString( "bordersize", "0" ) ).StripQuotes() ) );

	m_matScaleX->ChangeValue( WX( idStr( m_dict->GetString( "matscalex", "1" ) ).StripQuotes() ) );
	m_matScaleY->ChangeValue( WX( idStr( m_dict->GetString( "matscaley", "1" ) ).StripQuotes() ) );

	m_variableBackground->SetValue( m_dict->GetBool( "variablebackground", "0" ) );

	m_useMaterial->SetValue( !idStr( m_dict->GetString( "background", "" ) ).StripQuotes().IsEmpty() );
	m_useBackcolor->SetValue( !idStr( m_dict->GetString( "backcolor", "" ) ).StripQuotes().IsEmpty() );

	if( !idStr( m_dict->GetString( "borderShader", "" ) ).IsEmpty() )
	{
		m_useBorderMaterial->SetValue( true );
	}
	else
	{
		m_useBorderColor->SetValue( true );
	}

	UpdateCheckedStates();
	return true;
}

/*
================
rvGEItemPropsImagePage::KillActive
================
*/
bool rvGEItemPropsImagePage::KillActive()
{
	bool hasMat = false;

	if( m_useMaterial->GetValue() )
	{
		idStr s = "\"";
		s += m_background->GetValue().utf8_str();
		s += "\"";
		m_dict->Set( "background", s );
		hasMat = true;

		float vx = atof( m_matScaleX->GetValue().utf8_str() );
		if( vx >= 0.999f && vx <= 1.001f )
		{
			m_dict->Delete( "matscalex" );
		}
		else
		{
			m_dict->Set( "matscalex", idStr::FloatArrayToString( &vx, 1, 8 ) );
		}

		float vy = atof( m_matScaleY->GetValue().utf8_str() );
		if( vy >= 0.999f && vy <= 1.001f )
		{
			m_dict->Delete( "matscaley" );
		}
		else
		{
			m_dict->Set( "matscaley", idStr::FloatArrayToString( &vy, 1, 8 ) );
		}

		if( m_variableBackground->GetValue() )
		{
			m_dict->Set( "variablebackground", "1" );
		}
		else
		{
			m_dict->Delete( "variablebackground" );
		}
	}
	else
	{
		m_dict->Delete( "background" );
		m_dict->Delete( "variablebackground" );
		m_dict->Delete( "matscalex" );
		m_dict->Delete( "matscaley" );
	}

	if( m_useBackcolor->GetValue() )
	{
		const wxColour c = m_backColor->GetColour();
		const float a = m_backAlpha->GetAlpha();
		m_dict->Set( "backcolor",
					 StringFromVec4( idVec4( c.Red() / 255.0f, c.Green() / 255.0f,
											 c.Blue() / 255.0f, a ) ) );
	}
	else
	{
		m_dict->Delete( "backcolor" );
	}

	if( m_borderSize->GetValue() > 0 )
	{
		m_dict->Set( "bordersize", va( "%d", m_borderSize->GetValue() ) );

		if( m_useBorderColor->GetValue() )
		{
			const wxColour c = m_borderColor->GetColour();
			const float a = m_borderAlpha->GetAlpha();
			m_dict->Set( "bordercolor",
						 StringFromVec4( idVec4( c.Red() / 255.0f, c.Green() / 255.0f,
												 c.Blue() / 255.0f, a ) ) );
			m_dict->Delete( "borderShader" );
		}
		else
		{
			idStr s = "\"";
			s += m_borderMaterial->GetValue().utf8_str();
			s += "\"";
			m_dict->Set( "borderShader", s );
			m_dict->Delete( "bordercolor" );
			hasMat = true;
		}
	}
	else
	{
		m_dict->Delete( "bordersize" );
		m_dict->Delete( "bordercolor" );
		m_dict->Delete( "borderShader" );
	}

	if( hasMat )
	{
		const wxColour c = m_matColor->GetColour();
		const float a = m_matAlpha->GetAlpha();
		m_dict->Set( "matcolor",
					 StringFromVec4( idVec4( c.Red() / 255.0f, c.Green() / 255.0f,
											 c.Blue() / 255.0f, a ) ) );
	}
	else
	{
		m_dict->Delete( "matcolor" );
	}

	return true;
}

/*
================
rvGEItemPropsTextPage::rvGEItemPropsTextPage

Builds the "Text" page.
================
*/
rvGEItemPropsTextPage::rvGEItemPropsTextPage( wxWindow* parent, idDict* dict )
	: wxPanel( parent )
	, m_dict( dict )
{
	wxFlexGridSizer* sizer = new wxFlexGridSizer( 2, 6, 6 );
	sizer->AddGrowableCol( 1, 1 );

	m_useText = new wxCheckBox( this, wxID_ANY, "Use Text" );
	sizer->Add( m_useText, 0 );

	m_text = new wxTextCtrl( this, wxID_ANY, "", wxDefaultPosition, wxSize( -1, 60 ), wxTE_MULTILINE );
	sizer->Add( m_text, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Font:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_font = new wxComboBox( this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_font->Append( "<default>" );

	idFileList* folders = fileSystem->ListFiles( "fonts", "/" );
	for( int i = 0; i < folders->GetNumFiles(); ++i )
	{
		const char* f = folders->GetFile( i );
		if( f[0] == '.' )
		{
			continue;
		}
		m_font->Append( f );
	}
	fileSystem->FreeFileList( folders );

	sizer->Add( m_font, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Scale:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_scale = new wxTextCtrl( this, wxID_ANY );
	m_scale->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );
	sizer->Add( m_scale, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Forecolor:" ), 0, wxALIGN_CENTER_VERTICAL );
	wxBoxSizer* fcRow = new wxBoxSizer( wxHORIZONTAL );
	m_foreColor = new wxColourPickerCtrl( this, wxID_ANY );
	m_foreAlpha = new rvGEAlphaButton( this, wxID_ANY );
	fcRow->Add( m_foreColor, 1, wxEXPAND );
	fcRow->Add( m_foreAlpha, 0 );
	sizer->Add( fcRow, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Align:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_align = new wxComboBox( this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_align->Append( "Left" );
	m_align->Append( "Center" );
	m_align->Append( "Right" );
	sizer->Add( m_align, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Align X:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_alignX = new wxTextCtrl( this, wxID_ANY );
	m_alignX->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );
	sizer->Add( m_alignX, 1, wxEXPAND );

	sizer->Add( new wxStaticText( this, wxID_ANY, "Align Y:" ), 0, wxALIGN_CENTER_VERTICAL );
	m_alignY = new wxTextCtrl( this, wxID_ANY );
	m_alignY->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );
	sizer->Add( m_alignY, 1, wxEXPAND );

	sizer->AddSpacer( 4 );
	m_nowrap = new wxCheckBox( this, wxID_ANY, "No Wrap" );
	sizer->Add( m_nowrap, 0 );

	SetSizer( sizer );

	m_useText->Bind( wxEVT_CHECKBOX, &rvGEItemPropsTextPage::OnToggle, this );
}

/*
================
rvGEItemPropsTextPage::OnToggle
================
*/
void rvGEItemPropsTextPage::OnToggle( wxCommandEvent& )
{
	UpdateCheckedStates();
}

/*
================
rvGEItemPropsTextPage::UpdateCheckedStates
================
*/
void rvGEItemPropsTextPage::UpdateCheckedStates()
{
	const bool state = m_useText->GetValue();
	const bool foreEnabled = !IsExpression( m_dict->GetString( "forecolor", "1,1,1,1" ) );

	m_text->Enable( state );
	m_font->Enable( state );
	m_scale->Enable( state );
	m_align->Enable( state );
	m_alignX->Enable( state );
	m_alignY->Enable( state );
	m_nowrap->Enable( state );
	m_foreColor->Enable( state && foreEnabled );
	m_foreAlpha->Enable( state && foreEnabled );
}

/*
================
rvGEItemPropsTextPage::SetActive
================
*/
bool rvGEItemPropsTextPage::SetActive()
{
	auto parseVec4 = []( const char* s )
	{
		idVec4 v( 1, 1, 1, 1 );
		sscanf( s, "%f,%f,%f,%f", &v.x, &v.y, &v.z, &v.w );
		return v;
	};

	idVec4 fc = parseVec4( m_dict->GetString( "forecolor", "1,1,1,1" ) );
	m_foreColor->SetColour( wxColour(
								( unsigned char )( fc.x * 255 ), ( unsigned char )( fc.y * 255 ),
								( unsigned char )( fc.z * 255 ) ) );
	m_foreAlpha->SetAlpha( fc.w );

	m_text->ChangeValue( WX( idStr( m_dict->GetString( "text", "" ) ).StripQuotes() ) );
	m_scale->ChangeValue( WX( idStr( m_dict->GetString( "textscale", "1.0" ) ).StripQuotes() ) );
	m_alignX->ChangeValue( WX( idStr( m_dict->GetString( "textalignx", "0" ) ).StripQuotes() ) );
	m_alignY->ChangeValue( WX( idStr( m_dict->GetString( "textaligny", "0" ) ).StripQuotes() ) );

	m_align->SetSelection( atoi( m_dict->GetString( "textalign", "0" ) ) );

	idStr font = m_dict->GetString( "font", "" );
	font.StripQuotes();
	font.StripPath();
	int idx = m_font->FindString( WX( font ) );
	m_font->SetSelection( idx == wxNOT_FOUND ? 0 : idx );

	m_useText->SetValue( !idStr( m_dict->GetString( "text", "" ) ).IsEmpty() );
	m_nowrap->SetValue( atoi( idStr( m_dict->GetString( "nowrap", "0" ) ).StripQuotes() ) != 0 );

	UpdateCheckedStates();
	return true;
}

/*
================
rvGEItemPropsTextPage::KillActive
================
*/
bool rvGEItemPropsTextPage::KillActive()
{
	if( m_useText->GetValue() )
	{
		const wxColour c = m_foreColor->GetColour();
		const float a = m_foreAlpha->GetAlpha();
		m_dict->Set( "forecolor",
					 StringFromVec4( idVec4( c.Red() / 255.0f, c.Green() / 255.0f,
											 c.Blue() / 255.0f, a ) ) );

		idStr s = "\"";
		s += m_text->GetValue().utf8_str();
		s += "\"";
		m_dict->Set( "text", s );

		float f = atof( m_scale->GetValue().utf8_str() );
		m_dict->Set( "textscale", idStr::FloatArrayToString( &f, 1, 8 ) );

		const int alignSel = m_align->GetSelection();
		if( alignSel > 0 )
		{
			m_dict->Set( "textalign", va( "%d", alignSel ) );
		}
		else
		{
			m_dict->Delete( "textalign" );
		}

		int ax = atoi( m_alignX->GetValue().utf8_str() );
		if( ax )
		{
			m_dict->Set( "textalignx", va( "%d", ax ) );
		}
		else
		{
			m_dict->Delete( "textalignx" );
		}

		int ay = atoi( m_alignY->GetValue().utf8_str() );
		if( ay )
		{
			m_dict->Set( "textaligny", va( "%d", ay ) );
		}
		else
		{
			m_dict->Delete( "textaligny" );
		}

		const int fontSel = m_font->GetSelection();
		if( fontSel <= 0 )
		{
			m_dict->Delete( "font" );
		}
		else
		{
			m_dict->Set( "font", idStr( "\"fonts/" ) + m_font->GetString( fontSel ).utf8_str() + "\"" );
		}

		if( m_nowrap->GetValue() )
		{
			m_dict->Set( "nowrap", "1" );
		}
		else
		{
			m_dict->Delete( "nowrap" );
		}
	}
	else
	{
		m_dict->Delete( "text" );
		m_dict->Delete( "textscale" );
		m_dict->Delete( "textalign" );
		m_dict->Delete( "textalignx" );
		m_dict->Delete( "textaligny" );
		m_dict->Delete( "forecolor" );
		m_dict->Delete( "font" );
		m_dict->Delete( "nowrap" );
	}

	return true;
}

/*
================
rvGEItemPropsKeysPage::rvGEItemPropsKeysPage

Builds the "Keys" page: a list of raw key/value pairs.
================
*/
rvGEItemPropsKeysPage::rvGEItemPropsKeysPage( wxWindow* parent, idDict* dict, rvGEWindowWrapper* wrapper )
	: wxPanel( parent )
	, m_dict( dict )
	, m_wrapper( wrapper )
{
	m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES );
	m_list->InsertColumn( 0, "Key", wxLIST_FORMAT_LEFT, 120 );
	m_list->InsertColumn( 1, "Value", wxLIST_FORMAT_LEFT, 260 );

	wxButton* add = new wxButton( this, wxID_ADD, "&Add" );
	wxButton* edit = new wxButton( this, wxID_EDIT, "&Edit" );
	wxButton* remove = new wxButton( this, wxID_DELETE, "&Delete" );

	wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
	btnSizer->AddStretchSpacer();
	btnSizer->Add( add, 0, wxLEFT, 4 );
	btnSizer->Add( edit, 0, wxLEFT, 4 );
	btnSizer->Add( remove, 0, wxLEFT, 4 );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_list, 1, wxEXPAND );
	sizer->Add( btnSizer, 0, wxEXPAND | wxTOP, 6 );
	SetSizer( sizer );

	add->Bind( wxEVT_BUTTON, &rvGEItemPropsKeysPage::OnAddKey, this );
	edit->Bind( wxEVT_BUTTON, &rvGEItemPropsKeysPage::OnEditKey, this );
	remove->Bind( wxEVT_BUTTON, &rvGEItemPropsKeysPage::OnDeleteKey, this );

	m_list->Bind( wxEVT_LIST_ITEM_SELECTED, &rvGEItemPropsKeysPage::OnSelChanged, this );
	m_list->Bind( wxEVT_LIST_ITEM_DESELECTED, &rvGEItemPropsKeysPage::OnSelChanged, this );
}

/*
================
rvGEItemPropsKeysPage::KillActive

Keys page commits on every edit, nothing to flush.
================
*/
bool rvGEItemPropsKeysPage::KillActive()
{
	return true;
}

/*
================
rvGEItemPropsKeysPage::OnSelChanged
================
*/
void rvGEItemPropsKeysPage::OnSelChanged( wxListEvent& )
{
}

/*
================
rvGEItemPropsKeysPage::RefreshList

Repopulates the list control from the current dictionary contents.
================
*/
void rvGEItemPropsKeysPage::RefreshList()
{
	m_list->DeleteAllItems();
	if( !m_dict )
	{
		return;
	}

	for( int i = 0; i < m_dict->GetNumKeyVals(); ++i )
	{
		const idKeyValue* kv = m_dict->GetKeyVal( i );
		if( !kv )
		{
			continue;
		}

		const long row = m_list->InsertItem( m_list->GetItemCount(), kv->GetKey().c_str() );
		idStr v = kv->GetValue();
		v.StripQuotes();
		m_list->SetItem( row, 1, v.c_str() );
		m_list->SetItemData( row, i );
	}
}

/*
================
rvGEItemPropsKeysPage::SetActive
================
*/
bool rvGEItemPropsKeysPage::SetActive()
{
	RefreshList();
	return true;
}

/*
================
rvGEItemPropsKeysPage::EditKeyDialog

Modal add/edit dialog. Validates the value against the wrapper.
================
*/
bool rvGEItemPropsKeysPage::EditKeyDialog( const wxString& inKey, const wxString& inValue, bool allowKeyEdit, KeyEditResult& out )
{
	wxDialog dlg( this, wxID_ANY, allowKeyEdit ? "New Item Key" : "Modify Item Key", wxDefaultPosition, wxSize( 340, 180 ), wxDEFAULT_DIALOG_STYLE );

	wxTextCtrl* keyCtrl = new wxTextCtrl( &dlg, wxID_ANY, inKey );
	wxTextCtrl* valueCtrl = new wxTextCtrl( &dlg, wxID_ANY, inValue );
	keyCtrl->Enable( allowKeyEdit );
	keyCtrl->SetValidator( wxTextValidator( wxFILTER_EXCLUDE_CHAR_LIST ) );
	static_cast<wxTextValidator*>( keyCtrl->GetValidator() )->SetExcludes( BuildExcludeChars() );

	wxFlexGridSizer* grid = new wxFlexGridSizer( 2, 2, 6, 8 );
	grid->AddGrowableCol( 1, 1 );
	grid->Add( new wxStaticText( &dlg, wxID_ANY, "Key:" ), 0, wxALIGN_CENTER_VERTICAL );
	grid->Add( keyCtrl, 1, wxEXPAND );
	grid->Add( new wxStaticText( &dlg, wxID_ANY, "Value:" ), 0, wxALIGN_CENTER_VERTICAL );
	grid->Add( valueCtrl, 1, wxEXPAND );

	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( grid, 1, wxEXPAND | wxALL, 10 );
	sizer->Add( dlg.CreateButtonSizer( wxOK | wxCANCEL ), 0, wxEXPAND | wxALL, 6 );
	dlg.SetSizer( sizer );

	valueCtrl->SetFocus();
	valueCtrl->SetSelection( -1, -1 );

	while( dlg.ShowModal() == wxID_OK )
	{
		const wxString k = keyCtrl->GetValue();
		const wxString v = valueCtrl->GetValue();

		if( k.IsEmpty() )
		{
			wxMessageBox( "Key name cannot be empty.", "Invalid key", wxOK | wxICON_ERROR, &dlg );
			continue;
		}

		idStr finalValue = v.utf8_str();
		if( !m_wrapper->VerfiyStateKey( k.utf8_str(), finalValue ) )
		{
			idStr quoted = "\"";
			quoted += v.utf8_str();
			quoted += "\"";
			if( !m_wrapper->VerfiyStateKey( k.utf8_str(), quoted ) )
			{
				wxMessageBox( wxString::Format( "Invalid value '%s' for key '%s'.", v, k ), "Invalid value", wxOK | wxICON_ERROR, &dlg );
				continue;
			}
			finalValue = quoted;
		}

		out.key = k;
		out.value = finalValue.c_str();
		out.ok = true;
		return true;
	}

	out.ok = false;
	return false;
}

/*
================
rvGEItemPropsKeysPage::OnAddKey
================
*/
void rvGEItemPropsKeysPage::OnAddKey( wxCommandEvent& )
{
	KeyEditResult r;
	if( !EditKeyDialog( "", "", true, r ) )
	{
		return;
	}
	m_dict->Set( r.key.utf8_str(), r.value.utf8_str() );
	RefreshList();
}

/*
================
rvGEItemPropsKeysPage::OnEditKey
================
*/
void rvGEItemPropsKeysPage::OnEditKey( wxCommandEvent& )
{
	const long row = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if( row < 0 )
	{
		return;
	}

	const wxString oldKey = m_list->GetItemText( row );
	const wxString oldValue = m_list->GetItemText( row, 1 );
	const bool allowKeyEdit = wxStricmp( oldKey, "name" ) != 0 && wxStricmp( oldKey, "rect" ) != 0;

	KeyEditResult r;
	if( !EditKeyDialog( oldKey, oldValue, allowKeyEdit, r ) )
	{
		return;
	}

	if( !allowKeyEdit )
	{
		m_dict->Set( oldKey.utf8_str(), r.value.utf8_str() );
	}
	else
	{
		if( wxStricmp( r.key, oldKey ) != 0 )
		{
			m_dict->Delete( oldKey.utf8_str() );
		}
		m_dict->Set( r.key.utf8_str(), r.value.utf8_str() );
	}

	RefreshList();
}

/*
================
rvGEItemPropsKeysPage::OnDeleteKey
================
*/
void rvGEItemPropsKeysPage::OnDeleteKey( wxCommandEvent& )
{
	const long row = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if( row < 0 )
	{
		return;
	}

	const wxString key = m_list->GetItemText( row );
	if( wxStricmp( key, "name" ) == 0 || wxStricmp( key, "rect" ) == 0 )
	{
		wxBell();
		return;
	}

	m_dict->Delete( key.utf8_str() );
	RefreshList();
}

/*
================
GEItemPropsDlg_DoModal

Opens the item properties dialog and, on OK, writes the result into outDict.
================
*/
bool GEItemPropsDlg_DoModal( wxWindow* parent, idWindow* window, idDict& outDict )
{
	rvGEWindowWrapper* wrapper = rvGEWindowWrapper::GetWrapper( window );
	if( !wrapper )
	{
		return false;
	}

	outDict.Clear();
	outDict.Copy( wrapper->GetStateDict() );

	wxPropertySheetDialog dlg( parent, wxID_ANY, "Item Properties", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );
	dlg.CreateButtons( wxOK | wxCANCEL );

	rvGEItemPropsGeneralPage* general = new rvGEItemPropsGeneralPage( dlg.GetBookCtrl(), &outDict, wrapper->GetWindowType() );
	rvGEItemPropsImagePage* image =	new rvGEItemPropsImagePage( dlg.GetBookCtrl(), &outDict );
	rvGEItemPropsTextPage* text = new rvGEItemPropsTextPage( dlg.GetBookCtrl(), &outDict );
	rvGEItemPropsKeysPage* keys = new rvGEItemPropsKeysPage( dlg.GetBookCtrl(), &outDict, wrapper );

	dlg.GetBookCtrl()->AddPage( general, "General" );
	dlg.GetBookCtrl()->AddPage( image, "Image" );
	dlg.GetBookCtrl()->AddPage( text, "Text" );
	dlg.GetBookCtrl()->AddPage( keys, "Keys" );

	dlg.LayoutDialog();

	general->SetActive();

	dlg.GetBookCtrl()->Bind( wxEVT_BOOKCTRL_PAGE_CHANGING, [&]( wxBookCtrlEvent & e )
	{
		wxWindow* cur = dlg.GetBookCtrl()->GetPage( e.GetOldSelection() );
		if( cur == ( wxWindow* )general )
		{
			general->KillActive();
		}
		else if( cur == ( wxWindow* )image )
		{
			image->KillActive();
		}
		else if( cur == ( wxWindow* )text )
		{
			text->KillActive();
		}
		else if( cur == ( wxWindow* )keys )
		{
			keys->KillActive();
		}
	} );

	dlg.GetBookCtrl()->Bind( wxEVT_BOOKCTRL_PAGE_CHANGED, [&]( wxBookCtrlEvent & e )
	{
		wxWindow* page = dlg.GetBookCtrl()->GetPage( e.GetSelection() );
		if( page == ( wxWindow* )general )
		{
			general->SetActive();
		}
		else if( page == ( wxWindow* )image )
		{
			image->SetActive();
		}
		else if( page == ( wxWindow* )text )
		{
			text->SetActive();
		}
		else if( page == ( wxWindow* )keys )
		{
			keys->SetActive();
		}
	} );

	if( dlg.ShowModal() != wxID_OK )
	{
		return false;
	}

	const int sel = dlg.GetBookCtrl()->GetSelection();
	wxWindow* cur = dlg.GetBookCtrl()->GetPage( sel );
	if( cur == ( wxWindow* )general )
	{
		general->KillActive();
	}
	else if( cur == ( wxWindow* )image )
	{
		image->KillActive();
	}
	else if( cur == ( wxWindow* )text )
	{
		text->KillActive();
	}
	else if( cur == ( wxWindow* )keys )
	{
		keys->KillActive();
	}

	return true;
}