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

#include "FindDialog.h"
#include "MEMainFrame.h"

/*
================
FindDialog::FindDialog
================
*/
FindDialog::FindDialog( MEMainFrame* pParent )
	: wxDialog( pParent, wxID_ANY, "Find", wxDefaultPosition, wxSize( 260, 220 ), wxDEFAULT_DIALOG_STYLE )
	, m_parent( pParent )
	, m_textCtrl( NULL )
	, m_nameOnlyCheck( NULL )
	, m_searchFileRadio( NULL )
	, m_searchAllRadio( NULL )
{
	registry.Init( "findsettings.cfg", "FindDialog" );

	wxPanel* panel = new wxPanel( this );
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

	// Find what row
	{
		wxBoxSizer* row = new wxBoxSizer( wxHORIZONTAL );
		row->Add( new wxStaticText( panel, wxID_ANY, "Find What:" ), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4 );
		m_textCtrl = new wxTextCtrl( panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
		row->Add( m_textCtrl, 1, wxEXPAND | wxALL, 4 );
		sizer->Add( row, 0, wxEXPAND );
	}

	// Search scope group
	{
		wxStaticBoxSizer* group = new wxStaticBoxSizer( wxVERTICAL, panel, "Search" );
		m_searchFileRadio = new wxRadioButton( panel, wxID_ANY, "Current File", wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
		m_searchAllRadio  = new wxRadioButton( panel, wxID_ANY, "All Files" );
		m_nameOnlyCheck   = new wxCheckBox( panel, wxID_ANY, "Search Name Only" );

		group->Add( m_searchFileRadio, 0, wxALL, 4 );
		group->Add( m_searchAllRadio, 0, wxALL, 4 );
		group->Add( m_nameOnlyCheck, 0, wxALL, 4 );

		sizer->Add( group, 0, wxEXPAND | wxALL, 4 );
	}

	// Buttons
	{
		wxBoxSizer* row = new wxBoxSizer( wxHORIZONTAL );
		wxButton* findBtn  = new wxButton( panel, wxID_ANY, "Find Next" );
		wxButton* closeBtn = new wxButton( panel, ID_CloseButton, "Close" );
		row->Add( findBtn,  0, wxALL, 4 );
		row->Add( closeBtn, 0, wxALL, 4 );
		sizer->Add( row, 0, wxALIGN_RIGHT );

		findBtn->Bind( wxEVT_BUTTON,  &FindDialog::OnFindNext, this );
		closeBtn->Bind( wxEVT_BUTTON, &FindDialog::OnClose,    this );

		m_textCtrl->Bind( wxEVT_TEXT_ENTER, &FindDialog::OnFindNext, this );
	}

	panel->SetSizer( sizer );

	wxBoxSizer* outer = new wxBoxSizer( wxVERTICAL );
	outer->Add( panel, 1, wxEXPAND );
	SetSizer( outer );

	Bind( wxEVT_CLOSE_WINDOW, &FindDialog::OnCloseEvent, this );

	LoadFindSettings();
}

/*
================
FindDialog::~FindDialog
================
*/
FindDialog::~FindDialog()
{
}

/*
================
FindDialog::OnFindNext
================
*/
void FindDialog::OnFindNext( wxCommandEvent& )
{
	const wxString text = m_textCtrl->GetValue();

	if( text.IsEmpty() )
	{
		wxMessageBox( "Please enter a string to search for.", "Find", wxOK | wxICON_INFORMATION, this );
		m_textCtrl->SetFocus();
		return;
	}

	searchData.searchText = text.c_str();
	searchData.nameOnly = m_nameOnlyCheck->GetValue() ? 1 : 0;
	searchData.searchScope = m_searchFileRadio->GetValue() ? 0 : 1;
	searchData.searched = false;

	if( m_parent )
	{
		m_parent->FindNext( &searchData );
	}
}

/*
================
FindDialog::OnClose
================
*/
void FindDialog::OnClose( wxCommandEvent& )
{
	Close( true );
}

/*
================
FindDialog::OnCloseEvent
================
*/
void FindDialog::OnCloseEvent( wxCloseEvent& event )
{
	SaveFindSettings();

	if( m_parent )
	{
		m_parent->CloseFind();
	}

	Destroy();
}

/*
================
FindDialog::LoadFindSettings
================
*/
void FindDialog::LoadFindSettings()
{
	registry.Load();

	m_textCtrl->SetValue( registry.GetString( "searchText" ) );
	m_nameOnlyCheck->SetValue( registry.GetBool( "nameOnly", false ) );

	const int scope = ( int )registry.GetLong( "searchScope", 0 );
	if( scope == 0 )
	{
		m_searchFileRadio->SetValue( true );
	}
	else
	{
		m_searchAllRadio->SetValue( true );
	}

	registry.GetWindowPlacement( "findDialog", this );
}

/*
================
FindDialog::SaveFindSettings
================
*/
void FindDialog::SaveFindSettings()
{
	registry.SetString( "searchText", m_textCtrl->GetValue().c_str() );
	registry.SetBool( "nameOnly", m_nameOnlyCheck->GetValue() );
	registry.SetLong( "searchScope", m_searchFileRadio->GetValue() ? 0 : 1 );
	registry.SetWindowPlacement( "findDialog", this );
	registry.Save();
}