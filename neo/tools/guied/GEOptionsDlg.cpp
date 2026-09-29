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

class rvGEGeneralPage : public wxPanel
{
public:
	explicit rvGEGeneralPage( rvGEOptionsDlg* owner, wxWindow* parent )
		: wxPanel( parent, wxID_ANY )
	{
		wxFlexGridSizer* sizer = new wxFlexGridSizer( 2, 6, 8 );
		sizer->AddGrowableCol( 1, 1 );

		sizer->Add( new wxStaticText( this, wxID_ANY, "Selection Color:" ), 0, wxALIGN_CENTER_VERTICAL );
		owner->m_selectionColor = new wxColourPickerCtrl( this, wxID_ANY );
		sizer->Add( owner->m_selectionColor, 1, wxEXPAND );

		owner->m_ignoreDesktop = new wxCheckBox( this, wxID_ANY, "Ignore Desktop Select" );
		sizer->Add( owner->m_ignoreDesktop, 0, wxEXPAND );

		SetSizer( sizer );
	}
};

class rvGEGridPage : public wxPanel
{
public:
	explicit rvGEGridPage( rvGEOptionsDlg* owner, wxWindow* parent )
		: wxPanel( parent, wxID_ANY )
	{
		wxFlexGridSizer* sizer = new wxFlexGridSizer( 2, 6, 8 );
		sizer->AddGrowableCol( 1, 1 );

		owner->m_gridVisible = new wxCheckBox( this, wxID_ANY, "Show Grid" );
		owner->m_gridSnap = new wxCheckBox( this, wxID_ANY, "Snap To Grid" );
		sizer->Add( owner->m_gridVisible, 0, wxEXPAND );
		sizer->Add( owner->m_gridSnap, 0, wxEXPAND );

		sizer->Add( new wxStaticText( this, wxID_ANY, "Color:" ), 0, wxALIGN_CENTER_VERTICAL );
		owner->m_gridColor = new wxColourPickerCtrl( this, wxID_ANY );
		sizer->Add( owner->m_gridColor, 1, wxEXPAND );

		sizer->Add( new wxStaticText( this, wxID_ANY, "Spacing Width:" ), 0, wxALIGN_CENTER_VERTICAL );
		owner->m_spacingWidth = new wxSpinCtrl( this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1024, 10 );
		sizer->Add( owner->m_spacingWidth, 1, wxEXPAND );

		sizer->Add( new wxStaticText( this, wxID_ANY, "Spacing Height:" ), 0, wxALIGN_CENTER_VERTICAL );
		owner->m_spacingHeight = new wxSpinCtrl( this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1024, 10 );
		sizer->Add( owner->m_spacingHeight, 1, wxEXPAND );

		SetSizer( sizer );
	}
};

rvGEOptionsDlg::rvGEOptionsDlg( wxWindow* parent )
	: wxPropertySheetDialog( parent, wxID_ANY, "Options", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
	CreateButtons( wxOK | wxCANCEL );
	GetBookCtrl()->AddPage( new rvGEGeneralPage( this, GetBookCtrl() ), "General" );
	GetBookCtrl()->AddPage( new rvGEGridPage( this, GetBookCtrl() ), "Grid" );

	LayoutDialog();

	// Populate from options.
	rvGEOptions& opt = gApp->GetOptions();

	const idVec4& sc = opt.GetSelectionColor();
	m_selectionColor->SetColour( wxColour( ( unsigned char )( sc.x * 255 ), ( unsigned char )( sc.y * 255 ), ( unsigned char )( sc.z * 255 ) ) );
	m_ignoreDesktop->SetValue( opt.GetIgnoreDesktopSelect() );

	const idVec4& gc = opt.GetGridColor();
	m_gridColor->SetColour( wxColour( ( unsigned char )( gc.x * 255 ), ( unsigned char )( gc.y * 255 ), ( unsigned char )( gc.z * 255 ) ) );
	m_spacingWidth ->SetValue( opt.GetGridWidth() );
	m_spacingHeight->SetValue( opt.GetGridHeight() );
	m_gridVisible->SetValue( opt.GetGridVisible() );
	m_gridSnap->SetValue( opt.GetGridSnap() );

	Bind( wxEVT_BUTTON, &rvGEOptionsDlg::OnOK, this, wxID_OK );
}

void rvGEOptionsDlg::OnOK( wxCommandEvent& e )
{
	rvGEOptions& opt = gApp->GetOptions();

	const wxColour sc = m_selectionColor->GetColour();
	opt.SetSelectionColor( idVec4( sc.Red() / 255.0f, sc.Green() / 255.0f, sc.Blue() / 255.0f, 1.0f ) );
	opt.SetIgnoreDesktopSelect( m_ignoreDesktop->GetValue() );

	const wxColour gc = m_gridColor->GetColour();
	opt.SetGridColor( idVec4( gc.Red() / 255.0f, gc.Green() / 255.0f, gc.Blue() / 255.0f, 1.0f ) );
	opt.SetGridWidth( m_spacingWidth->GetValue() );
	opt.SetGridHeight( m_spacingHeight->GetValue() );
	opt.SetGridVisible( m_gridVisible->GetValue() );
	opt.SetGridSnap( m_gridSnap->GetValue() );

	e.Skip();
}